# Configuracoes Codex

Guarda aqui configuracoes locais do Codex, se existirem para este projeto.

## Como arrancar o Codex

1. Torna o script de arranque executável (uma vez):

   ```bash
   chmod +x Agentes/Codex/start_codex.sh
   ```

2. Arranca o Codex diretamente:

   ```bash
   ./Agentes/Codex/start_codex.sh
   ```

3. Para arrancar o Codex junto com o PlatformIO no VS Code:

   - abre a vista de Run/Debug (`Ctrl+Shift+D`).
   - escolhe a configuração `PIO Debug + Codex`.
   - inicia o debug.

O script `start_codex.sh` tenta primeiro executar `codex` e, se esse comando não existir, tenta `python3 -m codex`.

Se o Codex não estiver instalado, instala com:

```bash
pip install codex
```

O output do Codex é registado em `.vscode/codex.log`.

## Se a extensão do VS Code não aparecer como "Codex"

Se a extensão instalada no VS Code não aparecer com o nome "Codex", provavelmente estás a usar uma outra extensão relacionada, como:

- `OpenAI ChatGPT` (`openai.chatgpt`)

Para verificar a extensão instalada no terminal:

```bash
code --list-extensions | grep -i chatgpt
```

Se o VS Code não mostrar uma extensão chamada `codex`, usa a extensão instalada para acesso à funcionalidade de AI no editor, mas continua a usar o launcher local apenas se houver suporte a um comando `codex`.
