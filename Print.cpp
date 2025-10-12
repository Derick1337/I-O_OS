class Print {
public:
    static const char* state_to_string(const Process& p) {
        if (p.is_running)  return "EXECUTANDO";
        if (p.is_blocked)  return "BLOQUEADO";
        if (p.is_ready)    return "PRONTO";
        if (p.is_finished) return "FINALIZADO";
        return "DESCONHECIDO";
    }  

    void print_executing(const std::vector<Process> &all_processes) const {
        for (const auto &p : all_processes) {
            if (p.is_running) {
                std::cout << "---- RUNNING PROCESS ----\n";
                print_proc_line(&p);
                return;
            }
        }
        std::cout << "---- NO RUNNING PROCESS ----\n";
    }

    void print_ready_and_blocked(const std::vector<Process> &all_processes) const {
        print_by_state(all_processes,     "---- READY PROCESSES ----",
                       [](const Process& p) { return p.is_ready; });
        std::cout << "\n";
        print_by_state(all_processes,     "---- BLOCKED PROCESSES ----",
                       [](const Process& p) { return p.is_blocked; });
    }

    void print_devices(const std::vector<Device> &devices) const {
        std::cout << "---- DEVICES ----\n";
        if (devices.empty()) {
            std::cout << "(NO DEVICES)\n";
            return;
        }
        for (const auto& d : devices) {
            std::cout << "Device ID : " << d.name_id <<
                         " | Status Device: " << (d.is_busy ? "BUSY" : "FREE");
                if (!d.processes_using_devices.empty()) {
                    std::cout << " | Using: ";
                    print_pid_list(d.processes_using_devices);
                }
                if( !d.waiting_processes.empty()) {
                    std::cout << " | Waiting: ";
                    print_pid_queue(d.waiting_processes);
                }
                std::cout << "\n";
        }
    }
private:
    static void print_proc_line(const Process& p) {
        std::cout << "PID: " << p.pid
                  << " | Tempo de CPU restante: " << p.remaining_time
                  << " | Estado: " << state_to_string(p.state) << "\n";
    }
    template<typename Pred>
    void print_by(const std::vector<Process>& all_processes,
                  const char* title, Pred pred) const {
                    std::cout << title << "\n";
        bool any = false;
        for (const auto &p : all_processes) {
            if (pred(p)) {
                print_proc_line(p);
                any = true;
            }
        }
        if (!any) std::cout << "(none)\n";
    }

    static void print_pid_list(const std::vector<int>& pids){
        for (size_t i = 0; i < pids.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << pids[i];
        }
    }
    static void print_pid_queue(std::queue<int> q) { // copy to avoid consuming
        bool first = true;
        while (!q.empty()) {
            if (!first) std::cout << ", ";
            std::cout << q.front();
            q.pop();
            first = false;
        }
    }
};