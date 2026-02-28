#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cstdio>

class Logger {
public:
    // rows: total de linhas | stride: largura de cada linha (ex: 1024)
    Logger(size_t rows = 25, size_t stride = 128) 
        : rows_(rows), stride_(stride) {
        
        // Inicializa o stringzão com o tamanho exato e preenche com espaços
        buffer_.resize(rows_ * stride_, ' ');
        
        // Coloca um \n fixo no final de cada slot para o paragraph da UI
        for (size_t i = 1; i <= rows_; ++i) {
            buffer_[i * stride_ - 1] = '\n';
        }
    }

    ~Logger() {
        if (pipe_thread_.joinable()) {
            close(pipefds[0]);
            pipe_thread_.join();
        }
    }

   void start() {
        if (pipe(pipefds) == -1) return;

        original_stderr_fd = dup(STDERR_FILENO);

        dup2(pipefds[1], STDERR_FILENO);
        
        // IMPORTANTE: Desativar buffering no stderr redirecionado
        setvbuf(stderr, nullptr, _IONBF, 0);

        close(pipefds[1]); 

        pipe_thread_ = std::thread([this]() {
            // Usamos o descritor bruto para evitar o buffering da glibc (FILE*)
            int fd = pipefds[0];
            char c;
            std::string current_line;
            current_line.reserve(stride_);

            while (read(fd, &c, 1) > 0) {
                if (c == '\n') {
                    if (!current_line.empty()) {
                        addLog(current_line.c_str());
                        current_line.clear();
                    }
                } else {
                    current_line += c;
                    // Se a linha passar do limite do stride, quebra ela a força
                    if (current_line.size() >= stride_ - 2) {
                        addLog(current_line.c_str());
                        current_line.clear();
                    }
                }
            }
        });
    }

    void stop() {
        // Se temos um backup, restauramos ele para o lugar de direito
        if (original_stderr_fd != -1) {
            dup2(original_stderr_fd, STDERR_FILENO);
            close(original_stderr_fd);
            original_stderr_fd = -1;
        }
    }

    void addLog(const char* msg) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 1. O "SLIDE": Move da linha 1 até o fim para a posição da linha 0
        // memmove é seguro para áreas que se sobrepõem
        char* base_ptr = buffer_.data();
        std::memmove(base_ptr, 
                     base_ptr + stride_, 
                     (rows_ - 1) * stride_);

        // 2. O SLOT NOVO: É sempre a última linha
        char* last_line_ptr = base_ptr + (rows_ - 1) * stride_;

        // 3. LIMPEZA: Zera apenas a última linha (mantendo o \n no final)
        std::memset(last_line_ptr, ' ', stride_ - 1);

        // 4. ESCRITA: Copia a nova mensagem
        size_t len = std::min(std::strlen(msg), stride_ - 2); // -2 para espaço e \n
        std::memcpy(last_line_ptr, msg, len);
    }

    // Retorna a view do buffer inteiro, sempre ordenado.
    std::string_view getBufferView() const {
        return buffer_;
    }

private:
    std::string buffer_;
    size_t rows_, stride_;
    std::mutex mutex_;
    std::thread pipe_thread_;
    int pipefds[2];
    int original_stderr_fd = -1;
};