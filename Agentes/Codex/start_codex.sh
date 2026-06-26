#!/usr/bin/env bash
set -euo pipefail

# Ficheiro de verificação/arranque para o Codex no projeto.
# A extensão OpenAI do VS Code traz o seu próprio binário; em muitas instalações
# esse binário não existe como "codex" no PATH do terminal.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
EXTENSIONS_DIR="${HOME}/.vscode/extensions"

find_codex() {
    if [[ -n "${CODEX_BIN:-}" && -x "${CODEX_BIN}" ]]; then
        printf '%s\n' "${CODEX_BIN}"
        return 0
    fi

    if command -v codex >/dev/null 2>&1; then
        command -v codex
        return 0
    fi

    local bundled
    bundled="$(find "${EXTENSIONS_DIR}" -path '*/openai.chatgpt-*/bin/linux-aarch64/codex' -type f -perm -111 2>/dev/null | sort -V | tail -n 1 || true)"
    if [[ -n "${bundled}" ]]; then
        printf '%s\n' "${bundled}"
        return 0
    fi

    return 1
}

run_check() {
    echo "Workspace: ${WORKSPACE_DIR}"
    echo "Script: ${SCRIPT_DIR}"
    echo "PATH: ${PATH}"

    local codex_bin
    if ! codex_bin="$(find_codex)"; then
        echo "Erro: nao foi possivel encontrar o binario do Codex."
        echo "Instala/ativa a extensao OpenAI ChatGPT no VS Code ou define CODEX_BIN."
        return 1
    fi

    echo "Codex binario: ${codex_bin}"
    "${codex_bin}" --version
}

case "${1:-check}" in
    check|--check)
        run_check
        ;;
    start|run)
        shift || true
        codex_bin="$(find_codex)"
        echo "Iniciando Codex: ${codex_bin}"
        exec "${codex_bin}" "$@"
        ;;
    *)
        codex_bin="$(find_codex)"
        exec "${codex_bin}" "$@"
        ;;
esac
