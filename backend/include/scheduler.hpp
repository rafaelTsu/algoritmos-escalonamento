#pragma once

#include "process.hpp"

namespace sched {

enum class Algorithm {
    FCFS,
    SJF,
    PRIORITY,
    ROUND_ROBIN
};

struct Config {
    Algorithm algorithm;
    bool preemptive = false;
    int quantum = 0;
    int max_time = 0;          // 0 = sem limite; >0 = simula no maximo max_time ticks
};

SimulationResult run(const std::vector<Process>& processes, const Config& cfg);

const char* algorithm_name(Algorithm a);

}
