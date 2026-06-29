$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$miktexBin = Join-Path $env:LOCALAPPDATA "Programs\MiKTeX\miktex\bin\x64"
if (Test-Path $miktexBin) {
    $env:Path = "$miktexBin;$env:Path"
}

function Require-Command {
    param([string]$Name)

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) {
        throw "Comando '$Name' nao encontrado. Confirma se o MiKTeX esta instalado e se abriste um terminal novo."
    }
}

Require-Command "pdflatex"
Require-Command "biber"

function Invoke-LatexCommand {
    param(
        [string]$Command,
        [string[]]$Arguments
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "O comando '$Command $($Arguments -join ' ')' falhou com codigo $LASTEXITCODE."
    }
}

Invoke-LatexCommand "pdflatex" @("-interaction=nonstopmode", "main.tex")
Invoke-LatexCommand "biber" @("main")
Invoke-LatexCommand "pdflatex" @("-interaction=nonstopmode", "main.tex")
Invoke-LatexCommand "pdflatex" @("-interaction=nonstopmode", "main.tex")

$pdf = Join-Path $root "main.pdf"
if (-not (Test-Path $pdf)) {
    throw "A compilacao terminou, mas o ficheiro main.pdf nao foi encontrado."
}

$versionsFolderName = "vers" + [char]0x00F5 + "es"
$versionsDir = Join-Path $root $versionsFolderName
New-Item -ItemType Directory -Force $versionsDir | Out-Null

$existingVersions = Get-ChildItem $versionsDir -Filter "main_v*.pdf" -File -ErrorAction SilentlyContinue
$nextVersion = 1
foreach ($file in $existingVersions) {
    if ($file.BaseName -match '^main_v(\d+)$') {
        $version = [int]$Matches[1]
        if ($version -ge $nextVersion) {
            $nextVersion = $version + 1
        }
    }
}

$versionedPdf = Join-Path $versionsDir ("main_v{0}.pdf" -f $nextVersion)
Copy-Item -LiteralPath $pdf -Destination $versionedPdf

$buildDir = Join-Path $root "build"
New-Item -ItemType Directory -Force $buildDir | Out-Null

$auxiliaryPatterns = @(
    "main.aux",
    "main.bbl",
    "main.bcf",
    "main.blg",
    "main.lof",
    "main.log",
    "main.lot",
    "main.out",
    "main.run.xml",
    "main.toc"
)

foreach ($pattern in $auxiliaryPatterns) {
    $auxiliaryFile = Join-Path $root $pattern
    if (Test-Path -LiteralPath $auxiliaryFile) {
        Move-Item -LiteralPath $auxiliaryFile -Destination $buildDir -Force
    }
}

Write-Host "PDF compilado: $pdf"
Write-Host "Copia versionada criada: $versionedPdf"
Write-Host "Ficheiros auxiliares guardados em: $buildDir"
