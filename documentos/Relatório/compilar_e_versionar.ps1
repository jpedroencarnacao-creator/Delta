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

pdflatex -interaction=nonstopmode main.tex
biber main
pdflatex -interaction=nonstopmode main.tex
pdflatex -interaction=nonstopmode main.tex

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

Write-Host "PDF compilado: $pdf"
Write-Host "Copia versionada criada: $versionedPdf"
