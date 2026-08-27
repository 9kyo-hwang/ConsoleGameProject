#Requires -Version 5.1

[CmdletBinding()]
param(
    [string]$HostName = "127.0.0.1",
    [ValidateRange(1, 65535)]
    [int]$Port = 7777,
    [switch]$SplitSend,
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

    [byte[]]$header = Read-Exactly $stream 4
    $packetSize = Read-U16BigEndian $header 0
    $packetType = Read-U16BigEndian $header 2

    if ($packetSize -ne 10)
    {
        throw "Unexpected S2C_Enter packet size: $packetSize (expected 10)."
    }

    if ($packetType -ne 101)
    {
        throw "Unexpected packet type: $packetType (expected S2C_Enter=101)."
    }

    [byte[]]$payload = Read-Exactly $stream ($packetSize - 4)
    $version = Read-U16BigEndian $payload 0
    [uint32]$playerId = Read-U32BigEndian $payload 2

    if ($version -ne 1)
    {
        throw "Unexpected protocol version: $version (expected 1)."
    }

    $hex = (@($header) + @($payload) | ForEach-Object { $_.ToString("X2") }) -join " "
    Write-Output "PASS: S2C_Enter received (playerId=$playerId)"
    Write-Output "Packet: $hex"

    # C2S_Input:
    # size=10, type=2, sequence=1, direction=Up(1), actions=0
    [byte[]]$inputUp = 0x00, 0x0A, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00

    # sequence=2, direction=None(0), actions=0
    [byte[]]$inputStop = 0x00, 0x0A, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00

    $stream.Write($inputUp, 0, $inputUp.Length)
    Start-Sleep -Milliseconds 300

    $stream.Write($inputStop, 0, $inputStop.Length)
    Start-Sleep -Milliseconds 100
}
finally
{
    $client.Dispose()
}
