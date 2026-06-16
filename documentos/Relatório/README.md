# Relatório LaTeX

Estrutura base para o relatório do projeto final de licenciatura **Modelo Articulado de uma Árvore Brônquica**.

## Compilar

Com cópia automática para a pasta `versões/`:

```powershell
.\compilar_e_versionar.ps1
```

Este comando compila o relatório e guarda uma cópia do PDF como `versões/main_v1.pdf`, `versões/main_v2.pdf`, `versões/main_v3.pdf`, etc.

Com `latexmk`:

```powershell
latexmk -pdf -interaction=nonstopmode main.tex
```

Sem `latexmk`:

```powershell
pdflatex main.tex
biber main
pdflatex main.tex
pdflatex main.tex
```

## Estrutura

- `main.tex`: ficheiro principal do relatório.
- `capitulos/`: capítulos do corpo do relatório.
- `anexos/`: anexos do relatório.
- `bibliografia.bib`: referências extraídas dos PDFs, DOCX e `links.txt` presentes em `documentos/Bibliografia`.
- `notas-bibliografia.md`: mapa entre os documentos analisados e as entradas bibliográficas criadas.
- `compilar_e_versionar.ps1`: compila o relatório e guarda uma cópia versionada do PDF.
- `versões/`: histórico dos PDFs gerados.
- `figuras/`: imagens e esquemas.
- `tabelas/`: tabelas auxiliares.

As entradas de vídeo em `bibliografia.bib` devem ser confirmadas manualmente antes da versão final, porque o ficheiro `links.txt` contém apenas URLs.
