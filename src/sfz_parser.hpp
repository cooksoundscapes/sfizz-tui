#pragma once

#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <algorithm>
#include <filesystem>

#include "sfz_cache.hpp"

namespace fs = std::filesystem;

class SfzParser {
public:
    // Ponto de entrada principal
    static SfzMetaData load(const std::string& sfz_path) {
        auto cache_dir = get_cache_dir();
        std::string cache_path = generate_cache_path(sfz_path, cache_dir);
        uint64_t current_mtime = get_mtime(sfz_path);

        SfzMetaData cache;
        if (try_load_cache(cache_path, current_mtime, cache)) {
            return cache;
        }

        // Se falhou, faz o parse pesado
        cache = parse_file(sfz_path);
        cache.original_mtime = current_mtime;
        save_cache(cache_path, cache);
        
        return cache;
    }

private:
    static std::string get_cache_dir() {
        // A variável static garante que o bloco interno só rode UMA VEZ
        static const std::string cached_path = []() {
            const char* home = getenv("HOME");
            std::string dir = home ? std::string(home) + "/.cache/sfizz-tui" : "./.sfz_cache";
            
            // Cria a pasta apenas na primeira chamada da vida do programa
            if (!fs::exists(dir)) {
                try {
                    fs::create_directories(dir);
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "Erro crítico ao criar cache: " << e.what() << std::endl;
                }
            }
            return dir;
        }();

        return cached_path;
    }

    static uint64_t get_mtime(const std::string& path) {
        struct stat result;
        if (stat(path.c_str(), &result) == 0) return result.st_mtime;
        return 0;
    }

    // Gera o nome do arquivo usando um hash simples do path
    static std::string generate_cache_path(const std::string& path, const std::string& dir) {
        std::hash<std::string> hasher;
        return dir + "/" + std::to_string(hasher(path)) + ".blob";
    }

    static bool try_load_cache(const std::string& path, uint64_t mtime, SfzMetaData& out) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        f.read(reinterpret_cast<char*>(&out), sizeof(SfzMetaData));
        return out.original_mtime == mtime;
    }

    static void save_cache(const std::string& path, const SfzMetaData& data) {
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(&data), sizeof(SfzMetaData));
    }

    // Conversor de Nota (C4 -> 60)
    static int note_to_midi(std::string val) {
        if (val.empty()) return -1;
        if (isdigit(val[0])) return std::stoi(val);

        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        
        static const std::pair<std::string, int> notes[] = {
            {"c", 0}, {"c#", 1}, {"db", 1}, {"d", 2}, {"d#", 3}, {"eb", 3},
            {"e", 4}, {"f", 5}, {"f#", 6}, {"gb", 6}, {"g", 7}, {"g#", 8},
            {"ab", 8}, {"a", 9}, {"a#", 10}, {"bb", 10}, {"b", 11}
        };

        size_t i = 0;
        std::string name;
        while (i < val.size() && !isdigit(val[i]) && val[i] != '-') {
            name += val[i++];
        }
        
        int octave = std::stoi(val.substr(i));
        int base = 0;
        for (auto const& p : notes) if (p.first == name) { base = p.second; break; }

        return (octave + 1) * 12 + base;
    }

    static uint64_t string_to_tag(std::string_view tag) {
        // Função auxiliar local para comparação case-insensitive
        auto iequals = [](std::string_view a, std::string_view b) {
            return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                            [](char a, char b) { return std::tolower(a) == std::tolower(b); });
        };

        if (iequals(tag, "favorite"))   return Tag::FAVORITE;
        if (iequals(tag, "fav"))        return Tag::FAVORITE; // Atalho
        if (iequals(tag, "strings"))    return Tag::STRINGS;
        if (iequals(tag, "mellotron"))  return Tag::MELLOTRON;
        if (iequals(tag, "woodwind"))   return Tag::WOODWIND;
        if (iequals(tag, "brass"))      return Tag::BRASS;
        if (iequals(tag, "keys"))       return Tag::KEYS;
        if (iequals(tag, "tines"))      return Tag::TINES;
        if (iequals(tag, "perc"))       return Tag::PERC;
        if (iequals(tag, "portishead")) return Tag::PORTISHEAD;

        return Tag::NONE;
    }

    // O Parser de fato
    static SfzMetaData parse_file(const std::string& path) {
        SfzMetaData cache{};
        std::ifstream file(path);
        std::string line;
        
        int last_id = -1;
        std::string last_label = "";

        while (std::getline(file, line)) {
            // Procure por "// tags:" no início da linha
            if (line.compare(0, 8, "// tags:") == 0) {
                std::string content = line.substr(8);
                
                // Substitui pontuação comum por espaço para facilitar o split
                for (char& c : content) {
                    if (c == ',' || c == ';' || c == '.') c = ' ';
                }

                std::stringstream ss(content);
                std::string word;
                while (ss >> word) {
                    // Remove espaços em branco extras nas pontas
                    word.erase(word.find_last_not_of(" \t\r\n") + 1);
                    
                    uint64_t bit = string_to_tag(word);
                    if (bit != Tag::NONE) {
                        cache.tag_mask |= bit;
                    }
                }
                continue;
            }

            // Limpeza bruta: ignora comentários e linhas vazias
            if (line.empty() || line.find("//") == 0) continue;

            // Detecta Headers (reset de contexto local)
            if (line.find("<group>") != std::string::npos || line.find("<region>") != std::string::npos) {
                if (last_id != -1 && !last_label.empty()) {
                    cache.keyswitch_map[last_id].active = true;
                    strncpy(cache.keyswitch_map[last_id].label, last_label.c_str(), 31);
                }
            }

            // Opcodes de interesse
            size_t pos;
            if ((pos = line.find("sw_last=")) != std::string::npos) {
                std::string val = extract_value(line, pos + 8);
                last_id = note_to_midi(val);
            }
            if ((pos = line.find("sw_label=")) != std::string::npos) {
                last_label = extract_value(line, pos + 9);
            }
            if ((pos = line.find("sw_default=")) != std::string::npos) {
                std::string val = extract_value(line, pos + 11);
                cache.default_switch = note_to_midi(val);
            }
        }
        return cache;
    }

    static std::string extract_value(const std::string& line, size_t start) {
    // 1. Achar onde o valor realmente começa (pular espaços após o '=')
    size_t actual_start = line.find_first_not_of(" \t", start);
    if (actual_start == std::string::npos) return "";

    // 2. Se o valor estiver entre aspas, pegamos tudo entre elas
    if (line[actual_start] == '\"') {
        size_t closing_quote = line.find('\"', actual_start + 1);
        if (closing_quote != std::string::npos) {
            return line.substr(actual_start + 1, closing_quote - (actual_start + 1));
        }
    }

    // 3. Se NÃO tem aspas, o buraco é mais embaixo. 
    // No SFZ, labels podem ter espaços. O jeito mais seguro para a sua TUI 
    // é ler até o final da linha e limpar os espaços em branco do fim (trim).
    size_t end = line.find_last_not_of(" \t\r\n");
    if (end != std::string::npos && end >= actual_start) {
        return line.substr(actual_start, end - actual_start + 1);
    }

    return "";
}
};