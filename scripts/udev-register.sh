#!/bin/bash
# setup_udev.sh
RULE_PATH="/etc/udev/rules.d/99-sfizz-tui-autoconnect.rules"
RULE2_PATH="/etc/udev/rules.d/99-sfizz-tui-aconnect.rules"
APP_PATH=$(which sfizz-tui)

if [ -z "$APP_PATH" ]; then
    echo "Erro: sfizz-tui não está no PATH."
    exit 1
fi

# Generate UDEV rule for signaling main application..
echo "Criando regra udev em $RULE_PATH..."

cat <<EOF | sudo tee $RULE_PATH
ACTION=="bind", SUBSYSTEM=="snd_seq", DRIVER="snd_seq_midi, RUN+="/usr/bin/pkill -USR1 -f sfizz-tui"
EOF

# Generate UDEV rule for signaling external bash script as well - as a fallback
echo "Criando regra udev em $RULE2_PATH..."
CONNECT_SCRIPT="/usr/local/bin/sfizz-tui-aconnect.sh"

cat <<EOF | sudo tee $RULE2_PATH
ACTION=="bind", SUBSYSTEM=="snd_seq", DRIVER=="snd_seq_midi", RUN+="$CONNECT_SCRIPT"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger
echo "Pronto! O app agora responderá a hot-plug MIDI via SIGUSR1."