#!/bin/bash
# setup_udev.sh
RULE_PATH="/etc/udev/rules.d/99-sfizz-tui-autoconnect.rules"
APP_PATH=$(which sfizz-tui)

if [ -z "$APP_PATH" ]; then
    echo "Erro: sfizz-tui não está no PATH."
    exit 1
fi

echo "Criando regra udev em $RULE_PATH..."

# A regra envia SIGUSR1 para o processo pelo nome
cat <<EOF | sudo tee $RULE_PATH
ACTION=="bind", SUBSYSTEM=="snd_seq", DRIVER="snd_seq_midi, RUN+="/usr/bin/pkill -USR1 -f sfizz-tui"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger
echo "Pronto! O app agora responderá a hot-plug MIDI via SIGUSR1."