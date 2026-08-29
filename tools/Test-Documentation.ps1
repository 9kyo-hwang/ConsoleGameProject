#Requires -Version 5.1

[CmdletBinding()]
param(
    [switch]$StrictChangeAudit,
    [switch]$Staged
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$docsRoot = Join-Path $repositoryRoot "docs"
$failures = [System.Collections.Generic.List[string]]::new()
$strictUtf8 = [System.Text.UTF8Encoding]::new($false, $true)

function Add-Failure
{
    param([Parameter(Mandatory)][string]$Message)
    $failures.Add($Message)
}

function Test-MarkdownEncoding
{
    param([Parameter(Mandatory)][System.IO.FileInfo]$File)

    try
    {
        $text = $strictUtf8.GetString([System.IO.File]::ReadAllBytes($File.FullName))
        if ($text.Contains([char]0xFFFD))
        {
            Add-Failure "$($File.FullName): Unicode replacement character was found."
        }
    }
    catch [System.Text.DecoderFallbackException]
    {
        Add-Failure "$($File.FullName): File is not valid UTF-8."
    }
}

function Test-MarkdownLinks
{
    param([Parameter(Mandatory)][System.IO.FileInfo]$File)

    $text = [System.IO.File]::ReadAllText($File.FullName, $strictUtf8)
    $matches = [regex]::Matches($text, '!?' + '\[[^\]]*\]\((?<target>[^)]+)\)')

    foreach ($match in $matches)
    {
        $target = $match.Groups['target'].Value.Trim()
        if ($target.StartsWith('<') -and $target.EndsWith('>'))
        {
            $target = $target.Substring(1, $target.Length - 2)
        }

        if ([string]::IsNullOrWhiteSpace($target) -or
            $target.StartsWith('#') -or
            $target -match '^[a-zA-Z][a-zA-Z0-9+.-]*:')
        {
            continue
        }

        $pathPart = ($target -split '#', 2)[0]
        if ([string]::IsNullOrWhiteSpace($pathPart))
        {
            continue
        }

        $decodedPath = [System.Uri]::UnescapeDataString($pathPart)
        $resolvedPath = Join-Path $File.DirectoryName $decodedPath
        if (!(Test-Path -LiteralPath $resolvedPath))
        {
            Add-Failure "$($File.FullName): Missing relative link target '$target'."
        }
    }
}

function Get-ChangedPaths
{
    $arguments = @('-C', $repositoryRoot, 'diff')
    if ($Staged)
    {
        $arguments += '--cached'
    }

    $arguments += @('--name-only', '--diff-filter=ACMR', 'HEAD')
    [string[]]$changed = @(& git @arguments)
    if ($LASTEXITCODE -ne 0)
    {
        throw "git diff failed while collecting changed files."
    }

    if (!$Staged)
    {
        [string[]]$untracked = @(& git -C $repositoryRoot ls-files --others --exclude-standard)
        if ($LASTEXITCODE -ne 0)
        {
            throw "git ls-files failed while collecting untracked files."
        }

        $changed += $untracked
    }

    return @($changed | ForEach-Object { $_.Replace('\', '/') } | Sort-Object -Unique)
}

function Test-RequiredDocumentChange
{
    param(
        [Parameter(Mandatory)][string[]]$ChangedPaths,
        [Parameter(Mandatory)][string[]]$SourcePatterns,
        [Parameter(Mandatory)][string]$RequiredDocument
    )

    $sourceChanged = $false
    foreach ($path in $ChangedPaths)
    {
        foreach ($pattern in $SourcePatterns)
        {
            if ($path -like $pattern)
            {
                $sourceChanged = $true
                break
            }
        }

        if ($sourceChanged)
        {
            break
        }
    }

    if ($sourceChanged -and $ChangedPaths -notcontains $RequiredDocument)
    {
        Add-Failure "Changes matching '$($SourcePatterns -join ', ')' require '$RequiredDocument' to be updated."
    }
}

if (!(Test-Path -LiteralPath $docsRoot -PathType Container))
{
    throw "Documentation directory was not found: $docsRoot"
}

$markdownFiles = @(Get-ChildItem -LiteralPath $docsRoot -Recurse -File -Filter '*.md')
foreach ($file in $markdownFiles)
{
    Test-MarkdownEncoding $file
    Test-MarkdownLinks $file
}

if ($StrictChangeAudit)
{
    [string[]]$changedPaths = @(Get-ChangedPaths)

    Test-RequiredDocumentChange $changedPaths `
        @('Z1Shared/*', 'Z1Server/*', 'Z1/Network/*', 'tools/test-z1-enter.ps1') `
        'docs/Z1_MULTIPLAYER_IOCP_PLAN.md'

    Test-RequiredDocumentChange $changedPaths `
        @('*.slnx', '*.vcxproj', '*.vcxproj.filters', '*/*.vcxproj', '*/*.vcxproj.filters') `
        'docs/DEVELOPMENT.md'
}

if ($failures.Count -gt 0)
{
    foreach ($failure in $failures)
    {
        Write-Error $failure -ErrorAction Continue
    }

    exit 1
}

$auditLabel = if ($StrictChangeAudit) { ' with change audit' } else { '' }
Write-Output "PASS: $($markdownFiles.Count) Markdown files validated$auditLabel."
