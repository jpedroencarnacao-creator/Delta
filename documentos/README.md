# Documentos do Projeto

Organização das pastas de apoio ao projeto **Modelo Articulado de uma Árvore Brônquica**.

- `Bibliografia/`: artigos, documentos e links usados para fundamentação teórica.
- `Datasheets/`: folhas técnicas de componentes eletrónicos, mecânicos ou sensores.
- `Folha_Peças/`: listas de materiais, peças e custos do protótipo.
- `Imagens e Videos/`: registos fotograficos e videos do desenvolvimento.
  - `Imagens/`: fotografias e outras imagens.
  - `Videos/`: videos do projeto.
- `Relatório/`: fontes LaTeX, bibliografia, anexos e PDFs gerados.
  - `capitulos/`: capítulos principais do relatório.
  - `anexos/`: anexos do relatório.
  - `figuras/`: imagens usadas diretamente no relatório.
  - `tabelas/`: tabelas auxiliares usadas no relatório.
  - `versões/`: cópias versionadas do PDF gerado.
  - `build/`: ficheiros auxiliares criados pela compilação LaTeX.

Para compilar o relatório e guardar uma nova versão do PDF:

```powershell
cd documentos\Relatório
.\compilar_e_versionar.ps1
```
