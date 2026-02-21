#!/bin/bash
set -e

APP_NAME="Digital Workshop"
BIN_NAME="digital_workshop"
SETTINGS_BIN="dw_settings"
DESKTOP_ID="digitalworkshop"

# Default to user install, --system for /usr/local
PREFIX="$HOME/.local"
if [ "$1" = "--system" ]; then
    if [ "$(id -u)" -ne 0 ]; then
        echo "System uninstall requires root. Run with: sudo ./uninstall.sh --system"
        exit 1
    fi
    PREFIX="/usr/local"
fi

BIN_DIR="$PREFIX/bin"
DESKTOP_DIR="$HOME/.local/share/applications"
if [ "$1" = "--system" ]; then
    DESKTOP_DIR="/usr/share/applications"
fi

echo "Uninstalling $APP_NAME from $PREFIX..."

# Remove binaries
rm -f "$BIN_DIR/$BIN_NAME"
rm -f "$BIN_DIR/$SETTINGS_BIN"

# Remove desktop entry
rm -f "$DESKTOP_DIR/$DESKTOP_ID.desktop"

# Update desktop database if available
if command -v update-desktop-database > /dev/null 2>&1; then
    update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
fi

echo "$APP_NAME uninstalled successfully."
