#!/bin/bash

exec >> /tmp/sfizz-midi-connect.log 2>&1

# sfizz-midi-connect.sh
# instalaçao: sudo install -m 755 scripts/sfizz-tui-aconnect.sh /usr/local/bin/sfizz-tui-aconnect.sh

CONF="$HOME/.config/sfizz-tui/alsa-midi-connections.conf"
MIDI_THROUGH="14:0"

if [ ! -f "$CONF" ]; then
    echo "Conf não encontrado: $CONF"
    exit 1
fi

while IFS= read -r port_name; do
    # ignora linhas vazias e comentários
    [[ -z "$port_name" || "$port_name" == \#* ]] && continue

    port_id=$(aconnect -l | grep "$port_name" | grep -oP '^\s*\K\d+:\d+' | head -1)

    if [ -n "$port_id" ]; then
        aconnect "$port_id" "$MIDI_THROUGH" && echo "Conectado: $port_name ($port_id) → MIDI Through"
    else
        echo "Porta não encontrada: $port_name"
    fi
done < "$CONF"
```