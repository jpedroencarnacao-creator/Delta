#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="/home/airway/Documents/Projeto_Airway/Delta"
APP_FILE="$PROJECT_DIR/Python_CODE/Interface de controlo ver_PI4B/RB_PI4B_Main_PI.py"

cd "$PROJECT_DIR"

export LANG=C.UTF-8
export LC_ALL=C.UTF-8
export MPLCONFIGDIR="$PROJECT_DIR/Python_CODE/Interface de controlo ver_PI4B/.matplotlib-cache"

exec "$PROJECT_DIR/.venv/bin/python" -u "$APP_FILE"
