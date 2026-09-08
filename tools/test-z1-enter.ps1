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
    [switch]$ExpectEnemyPathDebug,
    [ValidateSet(-1, 0, 3, 4097)]
    [int]$InvalidPacketSize = -1,
    [ValidateRange(0, 65535)]
    [int]$ProtocolVersion = 5,
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

    if ($payload.Length -lt 8)
    {
        throw "WorldSnapshot payload is too small: $($payload.Length) bytes."
    }

    [uint32]$serverTick = Read-U32BigEndian $payload $offset
    $offset += 4
    [uint32]$actorCount = Read-U32BigEndian $payload $offset
    $offset += 4

    [int]$actorInfoSize = 15
    [int]$remaining = $payload.Length - $offset
    if ($actorCount -gt [math]::Floor($remaining / $actorInfoSize))
    {
        throw "WorldSnapshot actor array is truncated."
    }

    Write-Output "Snapshot: tick=$serverTick, actors=$actorCount"

    for ($index = 0; $index -lt $actorCount; ++$index)
    {
        [uint32]$actorId = Read-U32BigEndian $payload $offset
        $offset += 4
        [int]$hp = Read-I32BigEndian $payload $offset
        $offset += 4
        [uint16]$x = Read-U16BigEndian $payload $offset
        $offset += 2
        [uint16]$y = Read-U16BigEndian $payload $offset
        $offset += 2
        [int]$kind = $payload[$offset]
        ++$offset
        [int]$direction = $payload[$offset]
        ++$offset
        [int]$flags = $payload[$offset]
        ++$offset

        if ($actorId -eq 0)
        {
            throw "WorldSnapshot actor[$index] has invalid id=0."
        }

        switch ($kind)
        {
            1
            {
                $kindName = "Player"
                if (($direction -lt 0) -or ($direction -gt 4) -or (($flags -band 0xFC) -ne 0))
                {
                    throw "WorldSnapshot player actor[$index] is invalid."
                }

                if ($x -ne 1190 -or $y -ne 395)
                {
                    $script:SawPlayerMovement = $true
                }
            }
            2 { $kindName = "Enemy_Octorok" }
            3 { $kindName = "Enemy_Moblin" }
            4 { $kindName = "Enemy_Tektite" }
            5
            {
                $kindName = "Projectile_Spear"
                if ($direction -lt 1 -or $direction -gt 4)
                {
                    throw "WorldSnapshot projectile actor[$index] has invalid direction=$direction."
                }
            }
            default { throw "WorldSnapshot actor[$index] has unknown kind=$kind." }
        }

        if (($kind -ge 2) -and ($kind -le 4) -and
            (($direction -lt 0) -or ($direction -gt 4) -or (($flags -band 0xFE) -ne 0)))
        {
            throw "WorldSnapshot enemy actor[$index] is invalid."
        }

        Write-Output ("  Actor: id={0}, kind={1}({2}), position=({3}, {4}), direction={5}, hp={6}, flags={7}" -f $actorId, $kind, $kindName, $x, $y, $direction, $hp, $flags)
    }

    if ($offset -ne $payload.Length)
    {
        throw "WorldSnapshot has unexpected trailing bytes: $($payload.Length - $offset)."
    }

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

function Show-EnemyPathDebug
{
    param([Parameter(Mandatory)][pscustomobject]$Packet)

    if ($Packet.PacketType -ne 105)
    {
        throw "Expected S2C_EnemyPathDebug=105, received packet type $($Packet.PacketType)."
    }

    [byte[]]$payload = $Packet.Payload
    $fixedPayloadSize = 17 # tick, enemyId, roomX, roomY and count
    $roomCountX = 16
    $roomCountY = 8
    $roomTileCount = 16 * 11
    if ($payload.Length -lt $fixedPayloadSize)
    {
        throw "EnemyPathDebug payload is too small: $($payload.Length) bytes."
    }

    [uint32]$tick = Read-U32BigEndian $payload 0
    [uint32]$enemyId = Read-U32BigEndian $payload 4
    $roomX = Read-I32BigEndian $payload 8
    $roomY = Read-I32BigEndian $payload 12
    $count = [int]$payload[16]

    if ($enemyId -eq 0)
    {
        throw "EnemyPathDebug has invalid enemyId=0."
    }

    if ($roomX -lt 0 -or $roomX -ge $roomCountX -or
        $roomY -lt 0 -or $roomY -ge $roomCountY)
    {
        throw "EnemyPathDebug has invalid room=($roomX, $roomY)."
    }

    if ($count -gt $roomTileCount)
    {
        throw "EnemyPathDebug has invalid node count=$count."
    }

    if ($payload.Length - $fixedPayloadSize -ne $count)
    {
        throw "EnemyPathDebug has unexpected payload length: $($payload.Length) bytes for count=$count."
    }

    $indices = New-Object System.Collections.Generic.List[int]
    for ($index = 0; $index -lt $count; ++$index)
    {
        $tileIndex = [int]$payload[$fixedPayloadSize + $index]
        if ($tileIndex -ge $roomTileCount)
        {
            throw "EnemyPathDebug node[$index] has invalid tile index=$tileIndex."
        }

        [void]$indices.Add($tileIndex)
    }

    $pathText = if ($indices.Count -eq 0) { "<empty>" } else { ($indices -join ',') }
    Write-Output "PathDebug: tick=$tick, enemyId=$enemyId, room=($roomX, $roomY), nodes=$count, indices=$pathText"
}

if ($RunServerFramingSuite)
{
    if ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or
        $ExpectMovement -or $ExpectEnemyPathDebug -or $InvalidPacketSize -ne -1 -or $ProtocolVersion -ne 5 -or
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

if ($ExpectEnemyPathDebug -and
    (!$SendInput -or $InputDirection -eq 'None' -or $HoldMilliseconds -le 0 -or
     $SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or
     $SendAttack -or $ExpectMovement -or $ReadSnapshots -ne 0))
{
    throw "-ExpectEnemyPathDebug requires a non-empty -SendInput hold and cannot be combined with split, coalesced, attack, movement, or snapshot options."
}

if ($InvalidPacketSize -ne -1 -and
    ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or $SendInput -or
      $SendAttack -or $ExpectEnemyPathDebug -or $ProtocolVersion -ne 5 -or $ReadSnapshots -ne 0))
{
    throw "-InvalidPacketSize must be used by itself."
}

if ($ProtocolVersion -ne 5 -and
    ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or $SendInput -or
     $SendAttack -or $ExpectEnemyPathDebug -or $ReadSnapshots -ne 0))
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

    if ($ProtocolVersion -ne 5)
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

    if ($version -ne 5)
    {
        throw "Unexpected protocol version: $version (expected 5)."
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

            if ($packet.PacketType -eq 105)
            {
                Show-EnemyPathDebug $packet
                continue
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

    if ($ExpectEnemyPathDebug)
    {
        $sawEnemyPathDebug = $false
        for ($index = 0; $index -lt 200; ++$index)
        {
            $packet = Read-Packet $stream
            if ($packet.PacketType -eq 102)
            {
                continue
            }

            if ($packet.PacketType -eq 105)
            {
                Show-EnemyPathDebug $packet
                $sawEnemyPathDebug = $true
                break
            }

            throw "Unexpected packet type while waiting for EnemyPathDebug: $($packet.PacketType)"
        }

        if (!$sawEnemyPathDebug)
        {
            throw "S2C_EnemyPathDebug was not received within 200 packets."
        }

        Write-Output "PASS: S2C_EnemyPathDebug received."
    }

    $snapshotsRead = 0
    while ($snapshotsRead -lt $ReadSnapshots)
    {
        $snapshot = Read-Packet $stream
        if ($snapshot.PacketType -eq 102)
        {
            Show-WorldSnapshot $snapshot
            ++$snapshotsRead
        }
        elseif ($snapshot.PacketType -eq 105)
        {
            Show-EnemyPathDebug $snapshot
        }
        else
        {
            throw "Unexpected packet type while reading snapshots: $($snapshot.PacketType)"
        }
    }

    if($KeepAlive)
    {
        Write-Output "Dummy client connected. Press Ctrl+C to disconnect."

        while($true)
        {
            [void](Read-Packet $stream) # server packet 계속 drain
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
