#Requires -Version 5.1

[CmdletBinding()]
param(
    [string]$HostName = "127.0.0.1",
    [ValidateRange(1, 65535)]
    [int]$Port = 7777,
    [switch]$SplitSend,
    [switch]$SendInput,
    [ValidateSet("None", "Up", "Down", "Left", "Right")]
    [string]$InputDirection = "Up",
    [ValidateRange(0, 30000)]
    [int]$HoldMilliseconds = 300,
    [ValidateRange(0, 100)]
    [int]$ReadSnapshots = 0,
    [ValidateRange(100, 30000)]
    [int]$TimeoutMs = 3000
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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
        [byte]($Value -shr 24),
        [byte]($Value -shr 16),
        [byte]($Value -shr 8),
        [byte]$Value
    )
}

function Send-Input
{
    param(
        [Parameter(Mandatory)][System.IO.Stream]$Stream,
        [Parameter(Mandatory)][uint32]$Sequence,
        [Parameter(Mandatory)][string]$Direction
    )

    $directions = @{ None = 0; Up = 1; Down = 2; Left = 3; Right = 4 }
    [byte[]]$sequenceBytes = Get-U32BigEndianBytes $Sequence
    [byte[]]$packet = @(
        0x00, 0x0A, 0x00, 0x02,
        $sequenceBytes[0], $sequenceBytes[1],
        $sequenceBytes[2], $sequenceBytes[3],
        [byte]$directions[$Direction], 0x00
    )

    $Stream.Write($packet, 0, $packet.Length)
    Write-Output "Sent C2S_Input: sequence=$Sequence, direction=$Direction"
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

$client = [System.Net.Sockets.TcpClient]::new()

try
{
    $client.SendTimeout = $TimeoutMs
    $client.ReceiveTimeout = $TimeoutMs
    $client.Connect($HostName, $Port)

    $stream = $client.GetStream()
    $stream.ReadTimeout = $TimeoutMs
    $stream.WriteTimeout = $TimeoutMs

    # C2S_Enter: size=6, type=1, protocol version=1
    [byte[]]$enterPacket = 0x00, 0x06, 0x00, 0x01, 0x00, 0x01

    if ($SplitSend)
    {
        $stream.Write($enterPacket, 0, 2)
        Start-Sleep -Milliseconds 100
        $stream.Write($enterPacket, 2, 4)
    }
    else
    {
        $stream.Write($enterPacket, 0, $enterPacket.Length)
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

    for ($index = 0; $index -lt $ReadSnapshots; ++$index)
    {
        $snapshot = Read-Packet $stream
        Show-WorldSnapshot $snapshot
    }
}
finally
{
    $client.Dispose()
}
