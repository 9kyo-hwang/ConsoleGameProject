#Requires -Version 5.1

[CmdletBinding()]
param(
    [string]$HostName = "127.0.0.1",
    [ValidateRange(1, 65535)]
    [int]$Port = 7777,
    [switch]$SplitSend,
    [ValidateRange(0, 5)]
    [int]$SplitAt = 0,
    [switch]$CoalescedEnterInput,
    [switch]$RunServerFramingSuite,
    [switch]$ExpectMovement,
    [ValidateSet(-1, 0, 3, 4097)]
    [int]$InvalidPacketSize = -1,
    [ValidateRange(0, 65535)]
    [int]$ProtocolVersion = 4,
    [switch]$SendInput,
    [switch]$SendAttack,
    [ValidateSet("None", "Up", "Down", "Left", "Right")]
    [string]$InputDirection = "Up",
    [ValidateRange(0, 30000)]
    [int]$HoldMilliseconds = 300,
    [ValidateRange(0, 100)]
    [int]$ReadSnapshots = 0,
    [ValidateRange(100, 30000)]
    [int]$TimeoutMs = 3000,
    [switch]$KeepAlive
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$script:SawPlayerMovement = $false

function Read-Exactly
{
    param(
        [Parameter(Mandatory)][System.IO.Stream]$Stream,
        [Parameter(Mandatory)][int]$Count
    )

    [byte[]]$buffer = New-Object byte[] $Count
    $offset = 0

    while ($offset -lt $Count)
    {
        $read = $Stream.Read($buffer, $offset, $Count - $offset)
        if ($read -eq 0)
        {
            throw "Server closed the connection before the complete response arrived."
        }

        $offset += $read
    }

    return $buffer
}

function Read-U16BigEndian
{
    param([byte[]]$Bytes, [int]$Offset)
    return ([int]$Bytes[$Offset] -shl 8) -bor [int]$Bytes[$Offset + 1]
}

function Read-U32BigEndian
{
    param([byte[]]$Bytes, [int]$Offset)
    return ([uint32]$Bytes[$Offset] -shl 24) -bor
        ([uint32]$Bytes[$Offset + 1] -shl 16) -bor
        ([uint32]$Bytes[$Offset + 2] -shl 8) -bor
        [uint32]$Bytes[$Offset + 3]
}

function Read-I32BigEndian
{
    param([byte[]]$Bytes, [int]$Offset)

    [byte[]]$littleEndian = @(
        $Bytes[$Offset + 3],
        $Bytes[$Offset + 2],
        $Bytes[$Offset + 1],
        $Bytes[$Offset]
    )

    return [System.BitConverter]::ToInt32($littleEndian, 0)
}

function Read-Packet
{
    param([Parameter(Mandatory)][System.IO.Stream]$Stream)

    [byte[]]$header = Read-Exactly $Stream 4
    $packetSize = Read-U16BigEndian $header 0
    $packetType = Read-U16BigEndian $header 2

    if ($packetSize -lt 4 -or $packetSize -gt 4096)
    {
        throw "Invalid packet size received from server: $packetSize"
    }

    [byte[]]$payload = Read-Exactly $Stream ($packetSize - 4)

    return [pscustomobject]@{
        Header = $header
        PacketSize = $packetSize
        PacketType = $packetType
        Payload = $payload
    }
}

function Get-U32BigEndianBytes
{
    param([Parameter(Mandatory)][uint32]$Value)

    return [byte[]]@(
        [byte](($Value -shr 24) -band 0xFF),
        [byte](($Value -shr 16) -band 0xFF),
        [byte](($Value -shr 8) -band 0xFF),
        [byte]($Value -band 0xFF)
    )
}

function Get-U16BigEndianBytes
{
    param([Parameter(Mandatory)][uint16]$Value)

    return [byte[]]@(
        [byte](($Value -shr 8) -band 0xFF),
        [byte]($Value -band 0xFF)
    )
}

function New-InputPacket
{
    param(
        [Parameter(Mandatory)][uint32]$Sequence,
        [Parameter(Mandatory)][string]$Direction,
        [ValidateRange(0, 1)][byte]$ActionFlags = 0
    )

    $directions = @{ None = 0; Up = 1; Down = 2; Left = 3; Right = 4 }
    [byte[]]$sequenceBytes = Get-U32BigEndianBytes $Sequence

    return [byte[]]@(
        0x00, 0x0A, 0x00, 0x02,
        $sequenceBytes[0], $sequenceBytes[1],
        $sequenceBytes[2], $sequenceBytes[3],
        [byte]$directions[$Direction], $ActionFlags
    )
}

function Send-Input
{
    param(
        [Parameter(Mandatory)][System.IO.Stream]$Stream,
        [Parameter(Mandatory)][uint32]$Sequence,
        [Parameter(Mandatory)][string]$Direction,
        [ValidateRange(0, 1)][byte]$ActionFlags = 0
    )

    [byte[]]$packet = New-InputPacket $Sequence $Direction $ActionFlags

    $Stream.Write($packet, 0, $packet.Length)
    Write-Output "Sent C2S_Input: sequence=$Sequence, direction=$Direction, actionFlags=$ActionFlags"
}

function Assert-ServerClosedConnection
{
    param(
        [Parameter(Mandatory)][System.IO.Stream]$Stream,
        [Parameter(Mandatory)][string]$Scenario
    )

    try
    {
        $value = $Stream.ReadByte()
        if ($value -ne -1)
        {
            throw "$Scenario was accepted unexpectedly. First response byte: $value"
        }
    }
    catch [System.IO.IOException]
    {
        $socketError = $_.Exception.InnerException
        if ($socketError -is [System.Net.Sockets.SocketException] -and
            $socketError.SocketErrorCode -eq [System.Net.Sockets.SocketError]::TimedOut)
        {
            throw "$Scenario did not close the connection before timeout."
        }

        # ConnectionReset 등 서버가 연결을 끊으며 발생한 오류는 정상적인 거부 결과다.
    }

    Write-Output "PASS: Server rejected $Scenario"
}

function Show-WorldSnapshot
{
    param([Parameter(Mandatory)][pscustomobject]$Packet)

    if ($Packet.PacketType -ne 102)
    {
        throw "Expected S2C_WorldSnapshot=102, received packet type $($Packet.PacketType)."
    }

    [byte[]]$payload = $Packet.Payload
    $offset = 0

    if ($payload.Length -lt 10)
    {
        throw "WorldSnapshot payload is too small: $($payload.Length) bytes."
    }

    [uint32]$serverTick = Read-U32BigEndian $payload $offset
    $offset += 4
    $playerCount = Read-U16BigEndian $payload $offset
    $offset += 2

    Write-Output "Snapshot: tick=$serverTick, players=$playerCount"

    for ($index = 0; $index -lt $playerCount; ++$index)
    {
        if ($payload.Length - $offset -lt 18)
        {
            throw "WorldSnapshot player[$index] is truncated."
        }

        [uint32]$playerId = Read-U32BigEndian $payload $offset
        $offset += 4
        $x = Read-I32BigEndian $payload $offset
        $offset += 4
        $y = Read-I32BigEndian $payload $offset
        $offset += 4
        $facing = $payload[$offset]
        ++$offset
        $hp = Read-I32BigEndian $payload $offset
        $offset += 4
        $flags = $payload[$offset]
        ++$offset

        if ($x -ne 1190 -or $y -ne 395)
        {
            $script:SawPlayerMovement = $true
        }

        Write-Output ("  Player: id={0}, position=({1}, {2}), facing={3}, hp={4}, flags={5}" -f $playerId, $x, $y, $facing, $hp, $flags)
    }

    if ($payload.Length - $offset -lt 4)
    {
        throw "WorldSnapshot enemy/projectile count is truncated."
    }

    $enemyCount = Read-U16BigEndian $payload $offset
    $offset += 2

    # enemy 정보
    for ($index = 0; $index -lt $enemyCount; ++$index)
    {
        if ($payload.Length - $offset -lt 19)
        {
            throw "WorldSnapshot enemy[$index] is truncated."
        }

        [uint32]$enemyId = Read-U32BigEndian $payload $offset
        $offset += 4
        $kind = $payload[$offset]
        ++$offset
        $x = Read-I32BigEndian $payload $offset
        $offset += 4
        $y = Read-I32BigEndian $payload $offset
        $offset += 4
        $facing = $payload[$offset]
        ++$offset
        $hp = Read-I32BigEndian $payload $offset
        $offset += 4
        $flags = $payload[$offset]
        ++$offset

        if($enemyId -eq 0)
        {
            throw "WorldSnapshot enemy[$index] has invalid id=0."
        }

        if($kind -lt 0 -or $kind -gt 2)
        {
            throw "WorldSnapshot enemy[$index] has invalid kind=$kind"
        }

        if($facing -lt 0 -or $facing -gt 4)
        {
            throw "WorldSnapshot enemy[$index] has invalid facing=$facing"
        }

        # Attacking(0x01)만 허용 -> 1111 1110 검사했을 때 다른 비트 있으면 invlaid
        if(($flags -band 0xFE) -ne 0)
        {
            throw "WorldSnapshot enemy[$index] has invalid flags=$flags"
        }

        Write-Output ("  Enemy: id={0}, type={1}, position=({2}, {3}), facing={4}, hp={5}, flags={6}" -f $enemyId, $kind, $x, $y, $facing, $hp, $flags)
    }

    if($payload.Length - $offset -lt 2)
    {
        throw "WorldSnapshot projectile count is truncated."
    }

    $projectileCount = Read-U16BigEndian $payload $offset
    $offset += 2

    # projectile 정보
    for ($index = 0; $index -lt $projectileCount; ++$index)
    {
        if ($payload.Length - $offset -lt 14)
        {
            throw "WorldSnapshot projectile[$index] is truncated."
        }

        [uint32]$projectileId = Read-U32BigEndian $payload $offset
        $offset += 4
        $kind = $payload[$offset]
        ++$offset
        $x = Read-I32BigEndian $payload $offset
        $offset += 4
        $y = Read-I32BigEndian $payload $offset
        $offset += 4
        $direction = $payload[$offset]
        ++$offset

        if($projectileId -eq 0)
        {
            throw "WorldSnapshot projectile[$index] has invalid id=0."
        }

        if($kind -ne 0)
        {
            throw "WorldSnapshot projectile[$index] has invalid kind=$kind"
        }

        if($direction -lt 1 -or $direction -gt 4)
        {
            throw "WorldSnapshot projectile[$index] has invalid direction=$direction"
        }

        Write-Output ("  Projectile: id={0}, type={1}, position=({2}, {3}), direction={4}" -f $projectileId, $kind, $x, $y, $direction)
    }

    if ($offset -ne $payload.Length)
    {
        throw "WorldSnapshot has unexpected trailing bytes: $($payload.Length - $offset)."
    }

    Write-Output "  Enemies=$enemyCount, Projectiles=$projectileCount"
}

function Assert-CombatEvent
{
    param(
        [Parameter(Mandatory)][pscustomobject]$Packet,
        [Parameter(Mandatory)][uint32]$ExpectedActorId,
        [Parameter(Mandatory)][byte]$ExpectedDirection
    )

    if ($Packet.PacketType -ne 104)
    {
        throw "Expected S2C_CombatEvent=104, received packet type $($Packet.PacketType)."
    }

    if ($Packet.PacketSize -ne 10 -or $Packet.Payload.Length -ne 6)
    {
        throw "Unexpected S2C_CombatEvent packet size: $($Packet.PacketSize) (expected 10)."
    }

    [byte[]]$payload = $Packet.Payload
    $eventType = $payload[0]
    [uint32]$actorId = Read-U32BigEndian $payload 1
    $direction = $payload[5]

    if ($eventType -ne 1)
    {
        throw "CombatEvent has invalid event type: $eventType"
    }

    if ($actorId -ne $ExpectedActorId)
    {
        throw "CombatEvent actorId=$actorId does not match entered playerId=$ExpectedActorId."
    }

    if ($direction -ne $ExpectedDirection)
    {
        throw "CombatEvent direction=$direction does not match expected direction=$ExpectedDirection."
    }

    Write-Output "PASS: S2C_CombatEvent received (type=$eventType, actorId=$actorId, direction=$direction)"
}

if ($RunServerFramingSuite)
{
    if ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or
        $ExpectMovement -or $InvalidPacketSize -ne -1 -or $ProtocolVersion -ne 4 -or
        $SendInput -or $SendAttack -or $InputDirection -ne 'Up' -or
        $HoldMilliseconds -ne 300 -or $ReadSnapshots -ne 0)
    {
        throw "-RunServerFramingSuite cannot be combined with individual test scenario options."
    }

    $commonArguments = @{
        HostName = $HostName
        Port = $Port
        TimeoutMs = $TimeoutMs
    }

    $testCases = @(
        @{ Name = 'normal'; Arguments = @{ ReadSnapshots = 2 } },
        @{ Name = 'split-header'; Arguments = @{ SplitAt = 2; ReadSnapshots = 2 } },
        @{ Name = 'split-payload'; Arguments = @{ SplitAt = 4; ReadSnapshots = 2 } },
        @{ Name = 'coalesced'; Arguments = @{ CoalescedEnterInput = $true; ExpectMovement = $true; ReadSnapshots = 10 } },
        @{ Name = 'combat-event'; Arguments = @{ SendAttack = $true } },
        @{ Name = 'invalid-size-0'; Arguments = @{ InvalidPacketSize = 0 } },
        @{ Name = 'invalid-size-3'; Arguments = @{ InvalidPacketSize = 3 } },
        @{ Name = 'invalid-size-4097'; Arguments = @{ InvalidPacketSize = 4097 } },
        @{ Name = 'invalid-version'; Arguments = @{ ProtocolVersion = 1 } }
    )

    foreach ($testCase in $testCases)
    {
        Write-Output "=== $($testCase.Name) ==="

        $invokeArguments = @{} + $commonArguments
        foreach ($entry in $testCase.Arguments.GetEnumerator())
        {
            $invokeArguments[$entry.Key] = $entry.Value
        }

        & $PSCommandPath @invokeArguments
        if (!$?)
        {
            throw "Server framing suite failed: $($testCase.Name)"
        }
    }

    Write-Output "PASS: Server framing suite completed."
    return
}

if ($SplitSend -and $SplitAt -ne 0)
{
    throw "Use either -SplitSend or -SplitAt, not both."
}

if ($CoalescedEnterInput -and ($SplitSend -or $SplitAt -ne 0 -or $SendInput -or $SendAttack))
{
    throw "-CoalescedEnterInput cannot be combined with -SplitSend, -SplitAt, -SendInput, or -SendAttack."
}

if ($SendAttack -and ($SplitSend -or $SplitAt -ne 0 -or $SendInput -or $ReadSnapshots -ne 0))
{
    throw "-SendAttack cannot be combined with split, -SendInput, or -ReadSnapshots options."
}

if ($InvalidPacketSize -ne -1 -and
    ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or $SendInput -or
     $SendAttack -or $ProtocolVersion -ne 4 -or $ReadSnapshots -ne 0))
{
    throw "-InvalidPacketSize must be used by itself."
}

if ($ProtocolVersion -ne 4 -and
    ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or $SendInput -or
     $SendAttack -or $ReadSnapshots -ne 0))
{
    throw "A non-current -ProtocolVersion must be tested without split, input, or snapshot options."
}

$effectiveSplitAt = $SplitAt
if ($SplitSend)
{
    $effectiveSplitAt = 2
}

$client = [System.Net.Sockets.TcpClient]::new()

try
{
    $client.SendTimeout = $TimeoutMs
    $client.ReceiveTimeout = $TimeoutMs
    $client.Connect($HostName, $Port)

    $stream = $client.GetStream()
    $stream.ReadTimeout = $TimeoutMs
    $stream.WriteTimeout = $TimeoutMs

    if ($InvalidPacketSize -ne -1)
    {
        [byte[]]$invalidSizeBytes = Get-U16BigEndianBytes ([uint16]$InvalidPacketSize)
        [byte[]]$invalidPacket = @(
            $invalidSizeBytes[0], $invalidSizeBytes[1],
            0x00, 0x01
        )

        $stream.Write($invalidPacket, 0, $invalidPacket.Length)
        Assert-ServerClosedConnection $stream "packet size $InvalidPacketSize"
        return
    }

    # C2S_Enter: size=6, type=1, protocol version=ProtocolVersion
    [byte[]]$versionBytes = Get-U16BigEndianBytes ([uint16]$ProtocolVersion)
    [byte[]]$enterPacket = 0x00, 0x06, 0x00, 0x01, $versionBytes[0], $versionBytes[1]

    if ($CoalescedEnterInput)
    {
        [byte[]]$inputPacket = New-InputPacket 1 $InputDirection
        [byte[]]$combinedPacket = @($enterPacket) + @($inputPacket)
        $stream.Write($combinedPacket, 0, $combinedPacket.Length)
        Write-Output "Sent coalesced C2S_Enter + C2S_Input: direction=$InputDirection"
    }
    elseif ($effectiveSplitAt -ne 0)
    {
        $stream.Write($enterPacket, 0, $effectiveSplitAt)
        Start-Sleep -Milliseconds 100
        $stream.Write($enterPacket, $effectiveSplitAt, $enterPacket.Length - $effectiveSplitAt)
    }
    else
    {
        $stream.Write($enterPacket, 0, $enterPacket.Length)
    }

    if ($ProtocolVersion -ne 4)
    {
        Assert-ServerClosedConnection $stream "protocol version $ProtocolVersion"
        return
    }

    $enterResponse = Read-Packet $stream
    [byte[]]$header = $enterResponse.Header
    $packetSize = $enterResponse.PacketSize
    $packetType = $enterResponse.PacketType

    if ($packetSize -ne 10)
    {
        throw "Unexpected S2C_Enter packet size: $packetSize (expected 10)."
    }

    if ($packetType -ne 101)
    {
        throw "Unexpected packet type: $packetType (expected S2C_Enter=101)."
    }

    [byte[]]$payload = $enterResponse.Payload
    $version = Read-U16BigEndian $payload 0
    [uint32]$playerId = Read-U32BigEndian $payload 2

    if ($version -ne 4)
    {
        throw "Unexpected protocol version: $version (expected 4)."
    }

    $hex = (@($header) + @($payload) | ForEach-Object { $_.ToString("X2") }) -join " "
    Write-Output "PASS: S2C_Enter received (playerId=$playerId)"
    Write-Output "Packet: $hex"

    if ($SendInput)
    {
        Send-Input $stream 1 $InputDirection

        if ($InputDirection -ne "None" -and $HoldMilliseconds -gt 0)
        {
            Start-Sleep -Milliseconds $HoldMilliseconds
            Send-Input $stream 2 "None"
        }
    }
    elseif ($SendAttack)
    {
        Send-Input $stream 1 "None" 1

        $sawCombatEvent = $false
        for ($index = 0; $index -lt 20; ++$index)
        {
            $packet = Read-Packet $stream
            if ($packet.PacketType -eq 102)
            {
                Show-WorldSnapshot $packet
                continue
            }

            if ($packet.PacketType -eq 104)
            {
                Assert-CombatEvent $packet $playerId 1
                $sawCombatEvent = $true
                break
            }

            throw "Unexpected packet type while waiting for CombatEvent: $($packet.PacketType)"
        }

        if (!$sawCombatEvent)
        {
            throw "S2C_CombatEvent was not received within 20 packets."
        }
    }
    elseif ($CoalescedEnterInput -and $InputDirection -ne "None" -and $HoldMilliseconds -gt 0)
    {
        Start-Sleep -Milliseconds $HoldMilliseconds
        Send-Input $stream 2 "None"
    }

    for ($index = 0; $index -lt $ReadSnapshots; ++$index)
    {
        $snapshot = Read-Packet $stream
        Show-WorldSnapshot $snapshot
    }

    if($KeepAlive)
    {
        Write-Output "Dummy client connected. Press Ctrl+C to disconnect."

        while($true)
        {
            [void](Read-Packet $stream) # Snapshot 계속 drain
        }
    }

    if ($ExpectMovement)
    {
        if (!$script:SawPlayerMovement)
        {
            throw "No moved Player position was observed in the received snapshots."
        }

        Write-Output "PASS: Player movement was observed in a snapshot."
    }
}
finally
{
    $client.Dispose()
}
