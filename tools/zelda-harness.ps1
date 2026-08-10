#Requires -Version 5.1

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("status", "audit", "review", "help")]
    [string]$Action = "status",
    [switch]$Build
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$script:StatusPath = Join-Path $script:RepoRoot "docs\ZELDA_PROJECT_STATUS.md"
$script:Failures = 0
$script:Warnings = 0
$script:ExitCode = 0
Set-Location -LiteralPath $script:RepoRoot

function Get-CommandPath
{
    param([Parameter(Mandatory)][object]$Command)
    if (-not [string]::IsNullOrWhiteSpace($Command.Path)) { return $Command.Path }
    return $Command.Source
}

function Find-Command
{
    param([Parameter(Mandatory)][string]$Name)
    return Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1
}

function Run-External
{
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [string[]]$Arguments = @()
    )
    $previousErrorAction = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try
    {
        $output = & $FilePath @Arguments 2>&1 |
            ForEach-Object { $_.ToString() } | Out-String
    }
    finally
    {
        $ErrorActionPreference = $previousErrorAction
    }
    return [pscustomobject]@{
        ExitCode = $LASTEXITCODE
        Output = $output.Trim()
    }
}

function Write-Check
{
    param(
        [ValidateSet("PASS", "WARN", "FAIL", "INFO")]
        [string]$Kind,
        [Parameter(Mandatory)][string]$Message
    )
    if ($Kind -eq "FAIL") { $script:Failures++ }
    if ($Kind -eq "WARN") { $script:Warnings++ }
    Write-Output ("[{0}] {1}" -f $Kind, $Message)
}

function Write-OutputLines
{
    param([string]$Text, [int]$MaximumLines = 40)
    if ([string]::IsNullOrWhiteSpace($Text)) { return }
    $lines = $Text -split "\r?\n"
    $count = [Math]::Min($lines.Count, $MaximumLines)
    for ($index = 0; $index -lt $count; ++$index)
    {
        Write-Output ("    {0}" -f $lines[$index])
    }
    if ($lines.Count -gt $MaximumLines)
    {
        Write-Output ("    ... ({0}줄 생략)" -f ($lines.Count - $MaximumLines))
    }
}

function Get-StatusField
{
    param([Parameter(Mandatory)][string]$Label)
    if (-not (Test-Path -LiteralPath $script:StatusPath)) { return "(상태 문서 없음)" }

    $pattern = "^- {0}:\s*(.+)$" -f [regex]::Escape($Label)
    $line = Get-Content -LiteralPath $script:StatusPath -Encoding utf8 |
        Where-Object { $_ -match $pattern } | Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace([string]$line)) { return "(미기록)" }
    return ([regex]::Match([string]$line, $pattern)).Groups[1].Value.Trim()
}

function Show-Status
{
    Write-Output "=== ZeldaLikeGame Harness: status ==="
    Write-Output ("저장소: {0}" -f $script:RepoRoot)
    Write-Output ("단계: {0}" -f (Get-StatusField "현재 단계"))
    Write-Output ("작업: {0}" -f (Get-StatusField "현재 작업"))
    Write-Output ("상태: {0}" -f (Get-StatusField "상태"))
    Write-Output ("다음 작업: {0}" -f (Get-StatusField "다음 작업"))
    Write-Output ("마지막 검증: {0}" -f (Get-StatusField "마지막 검증"))
    Write-Output ""

    $git = Find-Command "git"
    if ($null -eq $git)
    {
        Write-Output "Git: 사용할 수 없음"
        $script:ExitCode = 1
        return
    }

    $gitPath = Get-CommandPath $git
    $status = Run-External $gitPath @("status", "--short")
    $stat = Run-External $gitPath @("-c", "core.safecrlf=false", "diff", "--stat")
    Write-Output "Git 변경 상태:"
    if ([string]::IsNullOrWhiteSpace($status.Output)) { Write-Output "  clean" }
    else { Write-OutputLines $status.Output 80 }
    Write-Output "Git diff 요약:"
    if ([string]::IsNullOrWhiteSpace($stat.Output)) { Write-Output "  변경된 tracked 파일 없음" }
    else { Write-OutputLines $stat.Output 40 }
    if ($status.ExitCode -ne 0 -or $stat.ExitCode -ne 0) { $script:ExitCode = 1 }
}

function Test-RequiredFiles
{
    $paths = @(
        "AGENTS.md",
        "ConsoleGameProject.slnx",
        "docs\ARCHITECTURE.md",
        "docs\DEVELOPMENT.md",
        "docs\ZELDA_LIKE_DEVELOPMENT_PLAN.md",
        "docs\ZELDA_MAP_DATA_REFERENCE.md",
        "docs\ZELDA_PROJECT_STATUS.md"
    )
    foreach ($relativePath in $paths)
    {
        if (Test-Path -LiteralPath (Join-Path $script:RepoRoot $relativePath))
        {
            Write-Check "PASS" ("필수 경로: {0}" -f $relativePath)
        }
        else
        {
            Write-Check "FAIL" ("필수 경로 없음: {0}" -f $relativePath)
        }
    }
}

function Test-Solution
{
    $path = Join-Path $script:RepoRoot "ConsoleGameProject.slnx"
    if (-not (Test-Path -LiteralPath $path)) { return }
    $text = Get-Content -LiteralPath $path -Raw -Encoding utf8
    $quote = [char]34
    $pattern = 'Project Path=' + $quote + '([^' + $quote + ']+)' + $quote
    $matches = [regex]::Matches($text, $pattern)
    if ($matches.Count -eq 0)
    {
        Write-Check "FAIL" "솔루션에 Project 항목이 없음"
        return
    }
    foreach ($match in $matches)
    {
        $relativePath = $match.Groups[1].Value.Replace([char]47, [char]92)
        $projectPath = Join-Path $script:RepoRoot $relativePath
        if (Test-Path -LiteralPath $projectPath)
        {
            Write-Check "PASS" ("솔루션 프로젝트 경로: {0}" -f $relativePath)
        }
        else
        {
            Write-Check "FAIL" ("솔루션 프로젝트 경로 불일치: {0}" -f $relativePath)
        }
    }
}

function Test-Assets
{
    $rg = Find-Command "rg"
    if ($null -eq $rg)
    {
        Write-Check "WARN" "rg를 찾을 수 없어 Assets 검사를 건너뜀"
        return
    }
    $targets = @(
        "CraftEngine", "ShootingGame", "SokobanGame",
        "docs", "README.md", "ConsoleGameProject.slnx"
    )
    $args = @("--line-number", "--color", "never", "Assets") + $targets
    $result = Run-External (Get-CommandPath $rg) $args
    if ($result.ExitCode -eq 0)
    {
        Write-Check "WARN" "활성 코드·문서에 Assets 문자열이 남아 있음"
        Write-OutputLines $result.Output 30
    }
    elseif ($result.ExitCode -eq 1)
    {
        Write-Check "PASS" "활성 코드·문서에 Assets 문자열이 없음"
    }
    else
    {
        Write-Check "WARN" "Assets 검사 중 rg 오류"
        Write-OutputLines $result.Output 20
    }
}

function Test-ZeldaProject
{
    $directory = Join-Path $script:RepoRoot "ZeldaLikeGame"
    if (-not (Test-Path -LiteralPath $directory))
    {
        Write-Check "INFO" "ZeldaLikeGame 프로젝트가 아직 없어 소스 등록 검사를 보류"
        return
    }

    $projectPath = Join-Path $directory "ZeldaLikeGame.vcxproj"
    $filtersPath = Join-Path $directory "ZeldaLikeGame.vcxproj.filters"
    if (-not (Test-Path -LiteralPath $projectPath))
    {
        Write-Check "FAIL" "ZeldaLikeGame.vcxproj가 없음"
        return
    }
    if (-not (Test-Path -LiteralPath $filtersPath))
    {
        Write-Check "FAIL" "ZeldaLikeGame.vcxproj.filters가 없음"
        return
    }

    $projectText = Get-Content -LiteralPath $projectPath -Raw -Encoding utf8
    $filtersText = Get-Content -LiteralPath $filtersPath -Raw -Encoding utf8
    $sourceFiles = @(Get-ChildItem -LiteralPath $directory -Recurse -File |
        Where-Object { $_.Extension -in @(".cpp", ".h") })
    foreach ($sourceFile in $sourceFiles)
    {
        $relativePath = $sourceFile.FullName.Substring($directory.Length + 1)
        $relativePath = $relativePath.Replace([char]92, [char]47)
        $include = 'Include=' + [char]34 + $relativePath + [char]34
        $projectHasFile = $projectText.Replace([char]92, [char]47).IndexOf(
            $include, [StringComparison]::OrdinalIgnoreCase) -ge 0
        $filtersHaveFile = $filtersText.Replace([char]92, [char]47).IndexOf(
            $include, [StringComparison]::OrdinalIgnoreCase) -ge 0
        if (-not $projectHasFile)
        {
            Write-Check "WARN" ("vcxproj 소스 미등록: {0}" -f $relativePath)
        }
        if (-not $filtersHaveFile)
        {
            Write-Check "WARN" ("filters 소스 미등록: {0}" -f $relativePath)
        }
    }

    if ($sourceFiles.Count -eq 0)
    {
        Write-Check "WARN" "ZeldaLikeGame에 C++ 소스가 없음"
    }
    else
    {
        Write-Check "PASS" ("ZeldaLikeGame 소스 등록 대상: {0}개" -f $sourceFiles.Count)
    }
}

function Test-ProjectSettings
{
    $projects = Get-ChildItem -LiteralPath $script:RepoRoot -Recurse -File -Filter "*.vcxproj" |
        Where-Object { $_.FullName -notmatch '\\(Binaries|Intermediate|\.vs)\\' }
    foreach ($project in $projects)
    {
        $text = Get-Content -LiteralPath $project.FullName -Raw -Encoding utf8
        $relativePath = $project.FullName.Substring($script:RepoRoot.Length + 1)
        if ($text -match "Assets")
        {
            Write-Check "WARN" ("프로젝트 설정에 Assets 문자열: {0}" -f $relativePath)
        }
        elseif ($text -match "Content")
        {
            Write-Check "PASS" ("프로젝트 Content 설정: {0}" -f $relativePath)
        }
    }
}

function Test-GitDiff
{
    $git = Find-Command "git"
    if ($null -eq $git)
    {
        Write-Check "FAIL" "git을 찾을 수 없음"
        return
    }
    $result = Run-External (Get-CommandPath $git) @("diff", "--check")
    if ($result.ExitCode -eq 0)
    {
        Write-Check "PASS" "git diff --check 통과"
    }
    else
    {
        Write-Check "FAIL" "git diff --check 실패"
        Write-OutputLines $result.Output 40
    }
}

function Invoke-BuildCheck
{
    $msbuild = Find-Command "msbuild"
    if ($null -eq $msbuild)
    {
        Write-Check "FAIL" "-Build 지정했지만 msbuild를 찾을 수 없음"
        return
    }
    Write-Check "INFO" "Debug|x64 솔루션 빌드 시작"
    $result = Run-External (Get-CommandPath $msbuild) @(
        "ConsoleGameProject.slnx",
        "/m",
        "/t:Build",
        "/p:Configuration=Debug",
        "/p:Platform=x64"
    )
    if ($result.ExitCode -eq 0)
    {
        Write-Check "PASS" "Debug|x64 솔루션 빌드 성공"
    }
    else
    {
        Write-Check "FAIL" "Debug|x64 솔루션 빌드 실패"
        Write-OutputLines $result.Output 80
    }
}

function Invoke-Audit
{
    Write-Output "=== ZeldaLikeGame Harness: audit ==="
    Test-RequiredFiles
    Test-Solution
    Test-Assets
    Test-ProjectSettings
    Test-ZeldaProject
    Test-GitDiff
    if ($Build) { Invoke-BuildCheck }
    else
    {
        Write-Check "INFO" "빌드는 실행하지 않음. 필요하면 audit -Build 사용"
    }
    Write-Output ""
    Write-Output ("감사 요약: FAIL={0}, WARN={1}" -f $script:Failures, $script:Warnings)
    if ($script:Failures -gt 0) { $script:ExitCode = 1 }
}

function Invoke-Review
{
    $codex = Find-Command "codex"
    if ($null -eq $codex)
    {
        Write-Check "FAIL" "codex CLI를 찾을 수 없음"
        $script:ExitCode = 1
        return
    }

    $status = (& $PSCommandPath "status" 2>&1 | Out-String).Trim()
    $audit = (& $PSCommandPath "audit" 2>&1 | Out-String).Trim()
    $promptLines = @(
        "이 저장소의 ZeldaLikeGame 개발 진행을 읽기 전용으로 감사하라.",
        "AGENTS.md와 docs/ZELDA_LIKE_DEVELOPMENT_PLAN.md를 반드시 읽어라.",
        "docs/ZELDA_MAP_DATA_REFERENCE.md, docs/ZELDA_PROJECT_STATUS.md,",
        "docs/ARCHITECTURE.md, docs/DEVELOPMENT.md도 읽어라.",
        "파일을 수정하거나 생성하지 말고, 확인하지 못한 빌드·실행은 완료로 추정하지 마라.",
        "Git 사실, 문서상 계획, 사용자가 직접 확인할 항목을 구분하고 다음 작업은 하나만 제시하라.",
        "",
        "하네스 status:",
        $status,
        "",
        "하네스 audit:",
        $audit,
        "",
        "한국어로 현재 단계, 완료 항목, 미검증 또는 위험 항목, 규칙 문제,",
        "다음 작업 하나와 완료 기준을 보고하라."
    )
    $prompt = $promptLines -join [Environment]::NewLine
    Write-Output "=== ZeldaLikeGame Harness: review ==="
    $result = Run-External (Get-CommandPath $codex) @(
        "exec", "--sandbox", "read-only", "--ephemeral", $prompt
    )
    if (-not [string]::IsNullOrWhiteSpace($result.Output))
    {
        Write-Output $result.Output
    }
    if ($result.ExitCode -ne 0)
    {
        Write-Check "FAIL" ("codex exec 실패 (exit code {0})" -f $result.ExitCode)
        $script:ExitCode = 1
    }
}

function Show-Help
{
    Write-Output "사용법:"
    Write-Output "  .\tools\zelda-harness.ps1 status"
    Write-Output "  .\tools\zelda-harness.ps1 audit"
    Write-Output "  .\tools\zelda-harness.ps1 audit -Build"
    Write-Output "  .\tools\zelda-harness.ps1 review"
}

switch ($Action)
{
    "status" { Show-Status }
    "audit" { Invoke-Audit }
    "review" { Invoke-Review }
    "help" { Show-Help }
}

exit $script:ExitCode
