#pragma once

#include <string>
#include <vector>

namespace sched {

enum class BurstType { CPU, IO };

struct Burst {
    BurstType type;
    int duration;
};

struct Process {
    int pid;
    int arrival;
    int priority;
    std::vector<Burst> bursts;
};

enum class State { NEW, READY, RUNNING, BLOCKED, FINISHED };

struct ProcessSnapshot {
    int pid;
    State state;
};

struct TickEvent {
    int time_start;
    int time_end;
    int running_pid;
    std::vector<ProcessSnapshot> snapshots;
    std::string note;
};

struct ProcessStats {
    int pid;
    int arrival;
    int completion;       // -1 se nao finalizou
    int turnaround;       // -1 se nao finalizou
    int waiting;          // tempo em READY (vale para finalizados e incompletos)
    int response;         // -1 se nunca executou
    bool finished;
    State final_state;    // estado em que terminou a simulacao
    int time_running;
    int time_ready;
    int time_blocked;
};

struct SimulationResult {
    std::vector<TickEvent> timeline;
    std::vector<ProcessStats> stats;
    std::vector<int> gantt;
    double avg_turnaround;     // media apenas sobre processos finalizados
    double avg_waiting;        // media apenas sobre processos finalizados
    double avg_response;       // media apenas sobre processos finalizados
    double cpu_utilization;
    int total_time;            // duracao total da simulacao em ticks
    int finished_count;
    int total_count;
    bool time_limit_reached;   // true se a simulacao parou por max_time, nao por conclusao
};

inline const char* state_name(State s) {
    switch (s) {
        case State::NEW:      return "NEW";
        case State::READY:    return "READY";
        case State::RUNNING:  return "RUNNING";
        case State::BLOCKED:  return "BLOCKED";
        case State::FINISHED: return "FINISHED";
    }
    return "?";
}

}
