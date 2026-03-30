#!/bin/bash
# setup_udev.sh
RULE_PATH="/etc/udev/rules.d/99-sfizz-tui-midi-signal.rules"
RULE2_PATH="/etc/udev/rules.d/99-sfizz-tui-alsa-connect.rules"
RULE3_PATH="/etc/udev/rules.d/99-sfizz-tui-bt-midi-connect.rules"

CONNECT_SCRIPT="/usr/local/bin/sfizz-tui-aconnect.sh"

APP_PATH=$(which sfizz-tui)

if [ -z "$APP_PATH" ]; then
    echo "Erro: sfizz-tui não está no PATH."
    exit 1
fi

# Generate UDEV rule for signaling main application..
echo "Criando regra udev em $RULE_PATH..."
cat <<EOF | sudo tee $RULE_PATH
ACTION=="bind", SUBSYSTEM=="snd_seq", DRIVER=="snd_seq_midi", RUN+="/usr/bin/pkill -USR1 -f sfizz-tui"
EOF

# Generate UDEV rule for signaling external bash script as well - as a fallback
echo "Criando regra udev em $RULE2_PATH..."
cat <<EOF | sudo tee $RULE2_PATH
ACTION=="bind", SUBSYSTEM=="snd_seq", DRIVER=="snd_seq_midi", RUN+="$CONNECT_SCRIPT"
EOF

# Generate UDEV rule for signaling external bash script as well - as a fallback
echo "Criando regra udev em $RULE3_PATH..."
cat <<EOF | sudo tee $RULE3_PATH
ACTION=="bind", SUBSYSTEM=="hid", ENV{HID_UNIQ}=="af:74:9b:34:da:2c", RUN+="$CONNECT_SCRIPT"
EOF

echo "Instalando o script de auto conexão via ALSA MIDI..."
install -m 755 scripts/sfizz-tui-aconnect.sh $CONNECT_SCRIPT

sudo udevadm control --reload-rules
echo "Pronto! O app agora responderá a hot-plug MIDI via SIGUSR1."