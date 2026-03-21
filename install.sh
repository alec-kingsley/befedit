#!/bin/bash

# shell coloring
CYAN='\e[0;36m'
YELLOW='\e[0;33m'
RED='\e[0;31m'
RESET='\e[0m'

PROMPT="${CYAN}$(basename "$0"):${RESET}"

function log_ok {
  echo -e "$PROMPT" "$@"
}

function prompt {
  echo -ne "$PROMPT" "$@"
}

function log_err {
  echo -e "$PROMPT" "${RED}Error:${RESET}" "$@"
}

function log_warn {
  echo -e "$PROMPT" "${YELLOW}Warning:${RESET}" "$@"
}

BEFEDIT_HOME=/usr/local/bin/befedit

function build_befedit {
  if ! make; then
    log_err "Failed to build befedit."
    exit 1
  fi
}

function install_befedit {
  if [ -e "$BEFEDIT_HOME" ]; then
    log_warn "befedit installation found"
    prompt "Overwrite? (y/n): "
    read -r overwrite
    if [ "$overwrite" = "y" ]; then
      log_ok "Removing previous installation..."
      sudo rm -rf "$BEFEDIT_HOME"
    else
      exit 1
    fi
  fi

  sudo cp bin/befedit "$BEFEDIT_HOME"
}

log_ok "Building befedit..."
build_befedit

log_ok "Installing..."
install_befedit

log_ok "Done!"
