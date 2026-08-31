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
    [int]$ProtocolVersion = 1,
    [switch]$SendInput,
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
        [Parameter(Mandatory)][string]$Direction
    )

    $directions = @{ None = 0; Up = 1; Down = 2; Left = 3; Right = 4 }
    [byte[]]$sequenceBytes = Get-U32BigEndianBytes $Sequence

    return [byte[]]@(
        0x00, 0x0A, 0x00, 0x02,
        $sequenceBytes[0], $sequenceBytes[1],
        $sequenceBytes[2], $sequenceBytes[3],
        [byte]$directions[$Direction], 0x00
    )
}

function Send-Input
{
    param(
        [Parameter(Mandatory)][System.IO.Stream]$Stream,
        [Parameter(Mandatory)][uint32]$Sequence,
        [Parameter(Mandatory)][string]$Direction
    )

    [byte[]]$packet = New-InputPacket $Sequence $Direction

    $Stream.Write($packet, 0, $packet.Length)
    Write-Output "Sent C2S_Input: sequence=$Sequence, direction=$Direction"
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
    $projectileCount = Read-U16BigEndian $payload $offset
    $offset += 2

    if ($offset -ne $payload.Length)
    {
        throw "WorldSnapshot has unexpected trailing bytes: $($payload.Length - $offset)."
    }

    Write-Output "  Enemies=$enemyCount, Projectiles=$projectileCount"
}

if ($RunServerFramingSuite)
{
    if ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or
        $ExpectMovement -or $InvalidPacketSize -ne -1 -or $ProtocolVersion -ne 1 -or
        $SendInput -or $InputDirection -ne 'Up' -or $HoldMilliseconds -ne 300 -or $ReadSnapshots -ne 0)
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
        @{ Name = 'invalid-size-0'; Arguments = @{ InvalidPacketSize = 0 } },
        @{ Name = 'invalid-size-3'; Arguments = @{ InvalidPacketSize = 3 } },
        @{ Name = 'invalid-size-4097'; Arguments = @{ InvalidPacketSize = 4097 } },
        @{ Name = 'invalid-version'; Arguments = @{ ProtocolVersion = 2 } }
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

if ($CoalescedEnterInput -and ($SplitSend -or $SplitAt -ne 0 -or $SendInput))
{
    throw "-CoalescedEnterInput cannot be combined with -SplitSend, -SplitAt, or -SendInput."
}

if ($InvalidPacketSize -ne -1 -and
    ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or $SendInput -or $ProtocolVersion -ne 1 -or $ReadSnapshots -ne 0))
{
    throw "-InvalidPacketSize must be used by itself."
}

if ($ProtocolVersion -ne 1 -and
    ($SplitSend -or $SplitAt -ne 0 -or $CoalescedEnterInput -or $SendInput -or $ReadSnapshots -ne 0))
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

    if ($ProtocolVersion -ne 1)
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

    if ($version -ne 1)
    {
        throw "Unexpected protocol version: $version (expected 1)."
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
