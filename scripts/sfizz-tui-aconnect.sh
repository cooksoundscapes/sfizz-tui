#!/bin/bash

exec >> /tmp/sfizz-midi-connect.log 2>&1

#ToDo: melhorar esse hardcode
CONF="/home/alarm/.config/sfizz-tui/alsa-midi-connections.conf"
MIDI_THROUGH="14:0"

if [ ! -f "$CONF" ]; then
    echo "Conf não encontrado: $CONF"
    exit 1
fi

while IFS= read -r port_name; do
    # ignora linhas vazias e comentários
    [[ -z "$port_name" || "$port_name" == \#* ]] && continue

    aconnect "$port_name" "$MIDI_THROUGH" && echo "Conectado: $port_name ($port_id) → MIDI Through"
done < "$CONF"
```