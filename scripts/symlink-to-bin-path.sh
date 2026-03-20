#!/bin/bash

EXEC_NAME=sfizz-tui
TARGET="/usr/bin/${EXEC_NAME}"
SOURCE="$(pwd)/build/${EXEC_NAME}"

# Remove o link se já existir
if [ -L "$TARGET" ] || [ -e "$TARGET" ]; then
    rm -f "$TARGET"
fi

# Cria o novo link
ln -s "$SOURCE" "$TARGET"