#!/usr/bin/env python
import subprocess
import os


# Caminho padrão XDG
MAP_DIR = os.path.expanduser("~/.config/sfizz-tui/mappings")

def get_jack_midi_ports():
    """Lista apenas portas de SAÍDA MIDI (onde as controladoras estão)."""
    try:
        # -p mostra propriedades, -t filtra por tipo
        output = subprocess.check_output(['jack_lsp', '-tp'], text=True)
    except FileNotFoundError:
        print("Erro: 'jack_lsp' não encontrado. O JACK está rodando?")
        return []

    ports = []
    current_port = ""
    is_midi = False
    is_output = False

    for line in output.splitlines():
        print(line, "<<<", line.startswith("  "))
        if not line[0].isspace(): # Linha de nome de porta
            current_port = line.strip()
            is_midi = False
            is_output = False
        else:
            if "properties: output" in line:
                is_output = True
            if "8 bit raw midi" in line: # Identificador de MIDI no JACK
                is_midi = True
            
            # Se for uma saída MIDI e não for o próprio sfizz-tui
            if is_midi and is_output and "sfizz-tui" not in current_port:
                ports.append(current_port)
                
    return sorted(list(set(ports)))

def create_map(port_full_name):
    if not os.path.exists(MAP_DIR):
        os.makedirs(MAP_DIR)

    # O JACK costuma dar nomes como "system:midi_capture_1" ou "Arturia:midi_out_1"
    # Vamos usar o nome da porta como chave de busca no .map
    safe_name = port_full_name.replace(":", "_").replace(" ", "_").lower()
    file_path = os.path.join(MAP_DIR, f"{safe_name}.map")

    if os.path.exists(file_path):
        print(f"[-] Mapa já existe: {file_path}")
        return

    content = f"""[device]
name = "{port_full_name}"
priority = 10

[mappings]
# <TYPE> <CH> <VALUE> <INTENT>
# examples:
# CC 1 114 BROWSE_MENU
# NOTE 10 36 CONFIRM_SELECT
"""
    with open(file_path, "w") as f:
        f.write(content)
    print(f"[+] Template gerado para porta: {port_full_name}")

if __name__ == "__main__":
    print("--- sfizz-tui JACK MIDI Scanner ---")
    ports = get_jack_midi_ports()
    
    if not ports:
        print("Nenhuma porta de saída MIDI encontrada no JACK.")
        print("Dica: Verifique se o a2jmidid está rodando.")
    else:
        for p in ports:
            create_map(p)