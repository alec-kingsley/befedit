#!/bin/bash

# shell coloring
CYAN='\e[0;36m'
RESET='\e[0m'

PROMPT="${CYAN}$(basename "$0"):${RESET}"

function log_ok {
  echo -e "$PROMPT" "$@"
}

BEFEDIT_HOME=/usr/local/bin/befedit
CONFIG_HOME=~/.config/befedit/config.b98

function uninstall_befedit {
  if [ -e "$BEFEDIT_HOME" ]; then
    sudo rm -rf "$BEFEDIT_HOME"
  fi

  if [ -e "${CONFIG_HOME%/*}" ]; then
    rm -rf "${CONFIG_HOME%/*}"
  fi
}

log_ok "Uninstalling..."
uninstall_befedit

log_ok "Done!"
