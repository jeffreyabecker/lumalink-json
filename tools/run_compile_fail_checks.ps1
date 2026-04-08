$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$casesRoot = Join-Path $repoRoot 'test\compile_fail'
$arduinoJsonHeader = Get-ChildItem -Path (Join-Path $repoRoot '.pio\libdeps') -Recurse -Filter 'ArduinoJson.h' -ErrorAction SilentlyContinue |
    Select-Object -First 1
$compilerCandidates = @('g++', 'clang++', 'c++')
$compiler = $null

if ($null -eq $arduinoJsonHeader) {
    throw 'ArduinoJson dependency headers were not found under .pio\libdeps. Run the native test environment once before the compile-fail harness.'
}

$arduinoJsonInclude = Split-Path -Parent $arduinoJsonHeader.FullName
$arduinoJsonSrcInclude = Join-Path $arduinoJsonInclude 'src'

foreach ($candidate in $compilerCandidates) {
    $command = Get-Command $candidate -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        $compiler = $command.Source
        break
    }
}

if ($null -eq $compiler) {
    throw 'No suitable C++ compiler found on PATH. Expected one of: g++, clang++, c++.'
}

$caseFiles = Get-ChildItem -Path $casesRoot -Filter '*.cpp' | Sort-Object Name
if ($caseFiles.Count -eq 0) {
    throw 'No compile-fail case files were found.'
}

$failedHarness = $false

foreach ($caseFile in $caseFiles) {
    $objectPath = Join-Path $env:TEMP ($caseFile.BaseName + '.o')
    $stdoutPath = Join-Path $env:TEMP ($caseFile.BaseName + '.stdout.txt')
    $stderrPath = Join-Path $env:TEMP ($caseFile.BaseName + '.stderr.txt')

    if (Test-Path $objectPath) {
        Remove-Item $objectPath -Force
    }
    if (Test-Path $stdoutPath) {
        Remove-Item $stdoutPath -Force
    }
    if (Test-Path $stderrPath) {
        Remove-Item $stderrPath -Force
    }

    $arguments = @(
        '-std=gnu++23'
        '-fno-exceptions'
        '-Isrc'
        '-Isrc/libs/pfr'
        '-Itest/compile_fail'
        "-I$arduinoJsonInclude"
        "-I$arduinoJsonSrcInclude"
        '-c'
        $caseFile.FullName
        '-o'
        $objectPath
    )

    $process = Start-Process -FilePath $compiler -ArgumentList $arguments -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath

    $exitCode = $process.ExitCode
    $stdout = if (Test-Path $stdoutPath) { Get-Content $stdoutPath -Raw } else { '' }
    $stderr = if (Test-Path $stderrPath) { Get-Content $stderrPath -Raw } else { '' }

    if ($stdout.Length -gt 0) {
        Write-Host $stdout
    }
    if ($stderr.Length -gt 0) {
        Write-Host $stderr
    }

    if ($exitCode -eq 0) {
        Write-Error "Expected compile failure, but '$($caseFile.Name)' compiled successfully."
        $failedHarness = $true
    }

    if (Test-Path $objectPath) {
        Remove-Item $objectPath -Force
    }
    if (Test-Path $stdoutPath) {
        Remove-Item $stdoutPath -Force
    }
    if (Test-Path $stderrPath) {
        Remove-Item $stderrPath -Force
    }
}

if ($failedHarness) {
    exit 1
}

Write-Host 'All compile-fail cases failed as expected.'
