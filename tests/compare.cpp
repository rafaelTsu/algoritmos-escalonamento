#include "scheduler.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace sched;

static std::string ganttString(const SimulationResult& r) {
    std::string s;
    int last = -2;
    int count = 0;
    auto flush = [&]() {
        if (last == -2) return;
        s += "[" + (last < 0 ? std::string("idle") : ("P" + std::to_string(last)))
             + "x" + std::to_string(count) + "] ";
    };
    for (int p : r.gantt) {
        if (p == last) count++;
        else { flush(); last = p; count = 1; }
    }
    flush();
    return s;
}

static void runCase(const std::string& title, const std::vector<Process>& ps, const Config& cfg) {
    auto r = run(ps, cfg);
    std::cout << std::left << std::setw(28) << title;
    std::cout << " | tempo=" << std::setw(3) << r.gantt.size()
              << " | TA=" << std::fixed << std::setprecision(2) << std::setw(5) << r.avg_turnaround
              << " | E=" << std::setw(5) << r.avg_waiting
              << " | R=" << std::setw(5) << r.avg_response
              << " | CPU%=" << std::setw(6) << (r.cpu_utilization * 100)
              << "\n";
    std::cout << "    Gantt: " << ganttString(r) << "\n";
    std::cout << "    ";
    for (const auto& s : r.stats) {
        std::cout << "P" << s.pid << "(TA=" << s.turnaround << ",E=" << s.waiting
                  << ",R=" << s.response << ") ";
    }
    std::cout << "\n\n";
}

int main() {
    std::vector<Process> caso1 = {
        {1, 0, 3, {{BurstType::CPU, 10}}},
        {2, 1, 1, {{BurstType::CPU, 4}}},
        {3, 2, 2, {{BurstType::CPU, 6}}},
        {4, 3, 4, {{BurstType::CPU, 2}}},
    };

    std::cout << "=========================================================\n";
    std::cout << " CASO 1: 4 processos, sem E/S\n";
    std::cout << " P1: arr=0 prio=3 CPU=10 | P2: arr=1 prio=1 CPU=4\n";
    std::cout << " P3: arr=2 prio=2 CPU=6  | P4: arr=3 prio=4 CPU=2\n";
    std::cout << "=========================================================\n\n";

    runCase("FCFS",                     caso1, {Algorithm::FCFS,        false, 0});
    runCase("SJF nao-preemptivo",       caso1, {Algorithm::SJF,         false, 0});
    runCase("SJF preemptivo (SRTF)",    caso1, {Algorithm::SJF,         true,  0});
    runCase("Prioridade nao-preempt.",  caso1, {Algorithm::PRIORITY,    false, 0});
    runCase("Prioridade preemptiva",    caso1, {Algorithm::PRIORITY,    true,  0});
    runCase("Round Robin (q=2)",        caso1, {Algorithm::ROUND_ROBIN, false, 2});
    runCase("Round Robin (q=4)",        caso1, {Algorithm::ROUND_ROBIN, false, 4});

    std::vector<Process> caso2 = {
        {1, 0, 2, {{BurstType::CPU, 4}, {BurstType::IO, 3}, {BurstType::CPU, 2}}},
        {2, 1, 1, {{BurstType::CPU, 5}}},
        {3, 3, 3, {{BurstType::CPU, 3}, {BurstType::IO, 2}, {BurstType::CPU, 2}}},
    };

    std::cout << "=========================================================\n";
    std::cout << " CASO 2: 3 processos com E/S\n";
    std::cout << " P1: arr=0 prio=2 [C4,I3,C2] | P2: arr=1 prio=1 [C5]\n";
    std::cout << " P3: arr=3 prio=3 [C3,I2,C2]\n";
    std::cout << "=========================================================\n\n";

    runCase("FCFS",                     caso2, {Algorithm::FCFS,        false, 0});
    runCase("SJF nao-preemptivo",       caso2, {Algorithm::SJF,         false, 0});
    runCase("SJF preemptivo (SRTF)",    caso2, {Algorithm::SJF,         true,  0});
    runCase("Prioridade nao-preempt.",  caso2, {Algorithm::PRIORITY,    false, 0});
    runCase("Prioridade preemptiva",    caso2, {Algorithm::PRIORITY,    true,  0});
    runCase("Round Robin (q=2)",        caso2, {Algorithm::ROUND_ROBIN, false, 2});

    return 0;
}
