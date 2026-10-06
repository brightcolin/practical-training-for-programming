param(
    [string]$Compiler = 'g++'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $repositoryRoot 'build/console'
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$jobs = @(
    @{ Name = 'CP_GetArea'; Sources = @('homework/01/CP_GetArea.cpp') },
    @{ Name = 'CP_MaxMain'; Sources = @('homework/01/CP_MaxMain.cpp') },
    @{ Name = 'CP_Teammate'; Sources = @('homework/01/CP_Teammate.cpp') },
    @{ Name = 'TwentyfourSolver'; Sources = @('homework/02/problem1/main.cpp', 'homework/02/problem1/Expression.cpp', 'homework/02/problem1/TwentyfourSolver.cpp') },
    @{ Name = 'TargetSolver'; Sources = @('homework/02/problem2/main.cpp', 'homework/02/problem2/TargetSolver.cpp') },
    @{ Name = 'CP_AnalysisTest'; Sources = @('homework/03/CP_Analysis.cpp', 'homework/03/CP_AnalysisTest.cpp') }
)

foreach ($job in $jobs) {
    $sources = @($job.Sources | ForEach-Object { Join-Path $repositoryRoot $_ })
    $destination = Join-Path $outputDirectory ($job.Name + '.exe')
    & $Compiler '-std=c++17' '-O2' @sources '-o' $destination
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: $($job.Name)"
    }
    Write-Host "Built $($job.Name)"
}
