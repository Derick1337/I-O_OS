#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

struct DeviceInfo
{
    std::string name;  // ID do dispositivo
    int capacity;      // Capacidade de usos simultâneos
    int access_time;   // Tempo de operação
};

struct ProcessInfo
{
    int creation_time = 0;
    int pid = 0;
    int execution_time = 0;
    int priority = 0;
    int memory_needed = 0;
    std::vector<int> page_sequence;
    int io_operations = 0; // Chance de solicitar E/S (0..100)
};

struct Config
{
    std::string scheduling_algorithm;
    int cpu_fraction = 0;
    std::string memory_policy;
    int memory_size = 0;
    int page_size = 0;
    double allocation_percentage = 0.0;
    int num_devices = 0;
};

bool read_file(const std::string &filename, Config &config, std::vector<DeviceInfo> &devices, std::vector<ProcessInfo> &processes)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return false;
    }

    std::string line;
    int line_count = 0;
    int devices_read = 0;

    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        line_count++;

        if (line_count == 1)
        {
            // Linha de configuração geral
            std::stringstream ss(line);
            std::string token;
            std::getline(ss, config.scheduling_algorithm, '|');
            std::getline(ss, token, '|');
            config.cpu_fraction = std::stoi(token);
            std::getline(ss, config.memory_policy, '|');
            std::getline(ss, token, '|');
            config.memory_size = std::stoi(token);
            std::getline(ss, token, '|');
            config.page_size = std::stoi(token);
            std::getline(ss, token, '|');
            config.allocation_percentage = std::stod(token);
            std::getline(ss, token);
            config.num_devices = std::stoi(token);
        }
        else if (devices_read < config.num_devices)
        {
            // Linhas de dispositivos
            DeviceInfo device;
            std::stringstream ss(line);
            std::string token;
            std::getline(ss, device.name, '|');
            std::getline(ss, token, '|');
            device.capacity = std::stoi(token);
            std::getline(ss, token);
            device.access_time = std::stoi(token);

            devices.push_back(device);
            devices_read++;
        }
        else
        {
            // Linhas de processos
            ProcessInfo process;
            std::stringstream ss(line);
            std::string token;

            std::getline(ss, token, '|');
            process.creation_time = std::stoi(token);
            std::getline(ss, token, '|');
            process.pid = std::stoi(token);
            std::getline(ss, token, '|');
            process.execution_time = std::stoi(token);
            std::getline(ss, token, '|');
            process.priority = std::stoi(token);
            std::getline(ss, token, '|');
            process.memory_needed = std::stoi(token);

            // Sequência de páginas (último campo antes de io_operations)
            std::getline(ss, token, '|');
            std::stringstream pages_ss(token);
            std::string page_str;

            while (std::getline(pages_ss, page_str, ',')) // suporta vírgula
            {
                if (page_str.empty()) continue;
                try
                {
                    process.page_sequence.push_back(std::stoi(page_str));
                }
                catch (...) {
                    std::cerr << "Aviso: número de página inválido '" << page_str << "' ignorado.\n";
                }
            }

            // Chance de requisitar E/S (pode estar ausente)
            if (std::getline(ss, token))
                process.io_operations = std::stoi(token);
            else
                process.io_operations = 0; // default

            processes.push_back(process);
        }
    }

    file.close();
    return true;
}

// Função de debug para imprimir o que foi lido
void print_debug(const Config &config, const std::vector<DeviceInfo> &devices, const std::vector<ProcessInfo> &processes)
{
    std::cout << "=== CONFIGURAÇÃO ===\n";
    std::cout << "Algoritmo: " << config.scheduling_algorithm << "\n";
    std::cout << "Quantum: " << config.cpu_fraction << "\n";
    std::cout << "Política Memória: " << config.memory_policy << "\n";
    std::cout << "Tamanho Memória: " << config.memory_size << "\n";
    std::cout << "Tamanho Página: " << config.page_size << "\n";
    std::cout << "Alocação: " << config.allocation_percentage << "%\n";
    std::cout << "Dispositivos: " << config.num_devices << "\n\n";

    std::cout << "=== DISPOSITIVOS ===\n";
    for (const auto &d : devices)
        std::cout << "ID: " << d.name << " | Capacidade: " << d.capacity << " | Tempo: " << d.access_time << "\n";

    std::cout << "\n=== PROCESSOS ===\n";
    for (const auto &p : processes)
    {
        std::cout << "PID: " << p.pid
                  << " | Criação: " << p.creation_time
                  << " | Execução: " << p.execution_time
                  << " | Prioridade: " << p.priority
                  << " | Memória: " << p.memory_needed
                  << " | Páginas: ";

        for (size_t i = 0; i < p.page_sequence.size(); ++i)
        {
            std::cout << p.page_sequence[i];
            if (i + 1 < p.page_sequence.size()) std::cout << ",";
        }

        std::cout << " | Chance E/S: " << p.io_operations << "%\n";
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Uso: " << argv[0] << " <arquivo_de_entrada>\n";
        return 1;
    }

    Config config;
    std::vector<DeviceInfo> devices;
    std::vector<ProcessInfo> processes;

    if (!read_file(argv[1], config, devices, processes))
        return 1;

    print_debug(config, devices, processes); // Para validar leitura antes da simulação
    return 0;
}
