#Requires -Version 5.1

[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$solutionPath = Join-Path $repositoryRoot 'ConsoleGameProject.slnx'
$failures = [System.Collections.Generic.List[string]]::new()

function Add-Failure
{
    param([Parameter(Mandatory)][string]$Message)
    $failures.Add($Message)
}

function Normalize-RelativePath
{
    param([Parameter(Mandatory)][string]$Path)
    return $Path.Replace('/', '\')
}

if (!(Test-Path -LiteralPath $solutionPath -PathType Leaf))
{
    throw "Solution file was not found: $solutionPath"
}

[xml]$solution = Get-Content -LiteralPath $solutionPath -Raw -Encoding utf8
$projectEntries = @($solution.Solution.Project)

foreach ($entry in $projectEntries)
{
    $relativeProjectPath = Normalize-RelativePath ([string]$entry.Path)
    $projectPath = Join-Path $repositoryRoot $relativeProjectPath

    if (!(Test-Path -LiteralPath $projectPath -PathType Leaf))
    {
        Add-Failure "Solution project does not exist: $relativeProjectPath"
        continue
    }

    $projectDirectory = Split-Path -Parent $projectPath
    $filtersPath = "$projectPath.filters"
    if (!(Test-Path -LiteralPath $filtersPath -PathType Leaf))
    {
        Add-Failure "Project filters file does not exist: $relativeProjectPath.filters"
        continue
    }

    [xml]$projectXml = Get-Content -LiteralPath $projectPath -Raw -Encoding utf8
    [xml]$filtersXml = Get-Content -LiteralPath $filtersPath -Raw -Encoding utf8

    [string[]]$projectIncludes = @($projectXml.SelectNodes(
        "//*[local-name()='ClCompile' or local-name()='ClInclude']") |
        ForEach-Object { $_.GetAttribute('Include') } |
        Where-Object { ![string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { Normalize-RelativePath $_ })

    [string[]]$filterIncludes = @($filtersXml.SelectNodes(
        "//*[local-name()='ClCompile' or local-name()='ClInclude']") |
        ForEach-Object { $_.GetAttribute('Include') } |
        Where-Object { ![string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { Normalize-RelativePath $_ })

    $sourceFiles = @(Get-ChildItem -LiteralPath $projectDirectory -Recurse -File |
        Where-Object { $_.Extension -in @('.cpp', '.h') })

    foreach ($sourceFile in $sourceFiles)
    {
        $relativeSourcePath = $sourceFile.FullName.Substring($projectDirectory.Length + 1)
        $relativeSourcePath = Normalize-RelativePath $relativeSourcePath

        if ($projectIncludes -notcontains $relativeSourcePath)
        {
            Add-Failure "$relativeProjectPath does not register source: $relativeSourcePath"
        }
        if ($filterIncludes -notcontains $relativeSourcePath)
        {
            Add-Failure "$relativeProjectPath.filters does not register source: $relativeSourcePath"
        }
    }

    foreach ($include in $projectIncludes)
    {
        $registeredPath = Join-Path $projectDirectory $include
        if (!(Test-Path -LiteralPath $registeredPath -PathType Leaf))
        {
            Add-Failure "$relativeProjectPath registers a missing file: $include"
        }
    }
}

if ($failures.Count -gt 0)
{
    foreach ($failure in $failures)
    {
        Write-Error $failure -ErrorAction Continue
    }
    exit 1
}

Write-Output "PASS: $($projectEntries.Count) solution projects and their source registrations validated."
