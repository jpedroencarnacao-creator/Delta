param(
    [Parameter(Position = 0)]
    [string]$Figura
)

$ErrorActionPreference = "Stop"

$relatorioDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$capitulosDir = Join-Path $relatorioDir "capitulos"
$outDir = Join-Path $relatorioDir "build\figuras_teste"

if (-not $Figura) {
    $Figura = Read-Host "Indica a figura a compilar, por exemplo 4.2 ou fig:fsm-motion"
}

if (-not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

function Get-FigureBlocks {
    param([string]$Text)

    $pattern = "(?s)\\begin\{figure\}(?:\[[^\]]*\])?.*?\\end\{figure\}"
    return [regex]::Matches($Text, $pattern)
}

$sourceFile = $null
$figureBlock = $null
$safeName = ($Figura -replace "[^\w.-]", "_")

if ($Figura -match "^(\d+)\.(\d+)$") {
    $chapterNumber = [int]$Matches[1]
    $figureIndex = [int]$Matches[2]
    $chapterPrefix = "{0:D2}-*.tex" -f $chapterNumber
    $chapterFile = Get-ChildItem -Path $capitulosDir -Filter $chapterPrefix | Select-Object -First 1

    if (-not $chapterFile) {
        throw "Nao encontrei o ficheiro do capitulo $chapterNumber em $capitulosDir."
    }

    $text = Get-Content -Path $chapterFile.FullName -Raw -Encoding UTF8
    $figures = Get-FigureBlocks -Text $text

    if ($figures.Count -lt $figureIndex) {
        throw "O capitulo $chapterNumber tem apenas $($figures.Count) figura(s)."
    }

    $sourceFile = $chapterFile.FullName
    $figureBlock = $figures[$figureIndex - 1].Value
}
else {
    $escapedLabel = [regex]::Escape($Figura)

    foreach ($file in Get-ChildItem -Path $capitulosDir -Filter "*.tex") {
        $text = Get-Content -Path $file.FullName -Raw -Encoding UTF8
        $figures = Get-FigureBlocks -Text $text

        foreach ($figure in $figures) {
            if ($figure.Value -match "\\label\{\s*$escapedLabel\s*\}" -or $figure.Value -match $escapedLabel) {
                $sourceFile = $file.FullName
                $figureBlock = $figure.Value
                break
            }
        }

        if ($figureBlock) {
            break
        }
    }

    if (-not $figureBlock) {
        throw "Nao encontrei nenhuma figura com '$Figura'. Usa o numero, como 4.2, ou o label, como fig:fsm-motion."
    }
}

$prefix = @'
\documentclass[a4paper,12pt]{article}

\usepackage[utf8]{inputenc}
\usepackage{lmodern}
\usepackage[margin=1cm]{geometry}
\usepackage{amsmath}
\usepackage{graphicx}
\usepackage{xcolor}
\usepackage{caption}
\usepackage{tikz}
\usetikzlibrary{arrows.meta,positioning,shapes.geometric,calc,fit}

\pagestyle{empty}

\begin{document}
'@

$suffix = @'

\end{document}
'@

$testTex = Join-Path $outDir ("teste_figura_$safeName.tex")
$testPdf = Join-Path $outDir ("teste_figura_$safeName.pdf")
$doc = $prefix + "`r`n" + $figureBlock + "`r`n" + $suffix

Set-Content -Path $testTex -Value $doc -Encoding UTF8

Push-Location $relatorioDir
try {
    & pdflatex -interaction=nonstopmode -halt-on-error -output-directory "build\figuras_teste" $testTex
}
finally {
    Pop-Location
}

Write-Host ""
Write-Host "Figura extraida de: $sourceFile"
Write-Host "Ficheiro de teste: $testTex"
Write-Host "PDF gerado: $testPdf"
