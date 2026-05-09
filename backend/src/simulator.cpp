#include "../include/scheduler.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace sched {

const char* algorithm_name(Algorithm a) {
    switch (a) {
        case Algorithm::FCFS:        return "FCFS";
        case Algorithm::SJF:         return "SJF";
        case Algorithm::PRIORITY:    return "Priority";
        case Algorithm::ROUND_ROBIN: return "Round Robin";
    }
    return "?";
}

namespace {

struct Runtime {
    Process proc;
    State state = State::NEW;
    std::size_t burst_idx = 0;
    int remaining_in_burst = 0;
    int quantum_left = 0;
    int ready_seq = 0;
    int first_run = -1;
    int completion = -1;
    int time_running = 0;
    int time_ready = 0;
    int time_blocked = 0;
};

bool eligible_for_cpu(const Runtime& r) {
    return r.state == State::READY || r.state == State::RUNNING;
}

int compare_running(const Runtime& a, const Runtime& b) {
    if (a.state == State::RUNNING && b.state != State::RUNNING) return 1;
    if (b.state == State::RUNNING && a.state != State::RUNNING) return -1;
    return 0;
}

bool better(const Runtime& a, const Runtime& b, const Config& cfg) {
    switch (cfg.algorithm) {
        case Algorithm::FCFS:
        case Algorithm::ROUND_ROBIN: {
            int rt = compare_running(a, b);
            if (rt > 0) return true;
            if (rt < 0) return false;
            if (a.ready_seq != b.ready_seq) return a.ready_seq < b.ready_seq;
            return a.proc.pid < b.proc.pid;
        }
        case Algorithm::SJF: {
            if (!cfg.preemptive) {
                int rt = compare_running(a, b);
                if (rt > 0) return true;
                if (rt < 0) return false;
            }
            if (a.remaining_in_burst != b.remaining_in_burst)
                return a.remaining_in_burst < b.remaining_in_burst;
            int rt = compare_running(a, b);
            if (rt > 0) return true;
            if (rt < 0) return false;
            return a.proc.pid < b.proc.pid;
        }
        case Algorithm::PRIORITY: {
            if (!cfg.preemptive) {
                int rt = compare_running(a, b);
                if (rt > 0) return true;
                if (rt < 0) return false;
            }
            if (a.proc.priority != b.proc.priority)
                return a.proc.priority < b.proc.priority;
            int rt = compare_running(a, b);
            if (rt > 0) return true;
            if (rt < 0) return false;
            return a.proc.pid < b.proc.pid;
        }
    }
    return false;
}

int select_runnable(const std::vector<Runtime>& rts, const Config& cfg) {
    int best = -1;
    for (std::size_t i = 0; i < rts.size(); i++) {
        if (!eligible_for_cpu(rts[i])) continue;
        if (best == -1 || better(rts[i], rts[best], cfg)) best = static_cast<int>(i);
    }
    return best;
}

bool any_alive(const std::vector<Runtime>& rts) {
    for (const auto& r : rts)
        if (r.state != State::FINISHED) return true;
    return false;
}

std::vector<ProcessSnapshot> snapshot(const std::vector<Runtime>& rts) {
    std::vector<ProcessSnapshot> snaps;
    snaps.reserve(rts.size());
    for (const auto& r : rts) snaps.push_back({r.proc.pid, r.state});
    return snaps;
}

}

SimulationResult run(const std::vector<Process>& processes, const Config& cfg) {
    std::vector<Runtime> rts;
    rts.reserve(processes.size());
    for (const auto& p : processes) {
        Runtime r;
        r.proc = p;
        r.state = State::NEW;
        r.remaining_in_burst = p.bursts.empty() ? 0 : p.bursts.front().duration;
        rts.push_back(r);
    }

    SimulationResult result;
    int t = 0;
    int ready_seq_counter = 0;
    int current = -1;

    const int MAX_TICKS = 1'000'000;
    const bool bounded = cfg.max_time > 0;
    const int hard_limit = bounded ? std::min(cfg.max_time, MAX_TICKS) : MAX_TICKS;

    while (any_alive(rts) && t < hard_limit) {
        for (auto& r : rts) {
            if (r.state == State::NEW && r.proc.arrival <= t) {
                r.state = State::READY;
                r.ready_seq = ready_seq_counter++;
            }
        }

        int chosen = select_runnable(rts, cfg);

        std::string note;
        if (chosen != current) {
            if (current != -1 && rts[current].state == State::RUNNING) {
                rts[current].state = State::READY;
                rts[current].ready_seq = ready_seq_counter++;
                rts[current].quantum_left = 0;
                if (chosen != -1) {
                    note = "P" + std::to_string(rts[current].proc.pid) +
                           " preemptado por P" + std::to_string(rts[chosen].proc.pid);
                } else {
                    note = "P" + std::to_string(rts[current].proc.pid) + " preemptado";
                }
            }
            if (chosen != -1) {
                rts[chosen].state = State::RUNNING;
                if (rts[chosen].first_run < 0) rts[chosen].first_run = t;
                if (cfg.algorithm == Algorithm::ROUND_ROBIN)
                    rts[chosen].quantum_left = cfg.quantum;
                if (note.empty())
                    note = "P" + std::to_string(rts[chosen].proc.pid) + " inicia execução";
            } else if (note.empty()) {
                note = "CPU ociosa";
            }
            current = chosen;
        }

        TickEvent ev;
        ev.time_start = t;
        ev.running_pid = (chosen == -1) ? -1 : rts[chosen].proc.pid;
        ev.snapshots = snapshot(rts);
        ev.note = note;

        for (auto& r : rts) {
            switch (r.state) {
                case State::RUNNING: r.time_running++; break;
                case State::READY:   r.time_ready++;   break;
                case State::BLOCKED: r.time_blocked++; break;
                default: break;
            }
        }

        if (chosen != -1) {
            rts[chosen].remaining_in_burst--;
            if (cfg.algorithm == Algorithm::ROUND_ROBIN)
                rts[chosen].quantum_left--;
        }
        for (auto& r : rts) {
            if (r.state == State::BLOCKED) r.remaining_in_burst--;
        }

        t++;
        ev.time_end = t;
        result.timeline.push_back(ev);
        result.gantt.push_back(ev.running_pid);

        if (chosen != -1 && rts[chosen].remaining_in_burst == 0) {
            auto& r = rts[chosen];
            r.burst_idx++;
            if (r.burst_idx >= r.proc.bursts.size()) {
                r.state = State::FINISHED;
                r.completion = t;
                current = -1;
            } else {
                const auto& nb = r.proc.bursts[r.burst_idx];
                r.remaining_in_burst = nb.duration;
                if (nb.type == BurstType::IO) {
                    r.state = State::BLOCKED;
                    current = -1;
                }
            }
        } else if (cfg.algorithm == Algorithm::ROUND_ROBIN
                   && chosen != -1
                   && rts[chosen].quantum_left == 0
                   && rts[chosen].state == State::RUNNING) {
            rts[chosen].state = State::READY;
            rts[chosen].ready_seq = ready_seq_counter++;
            current = -1;
        }

        for (auto& r : rts) {
            if (r.state == State::BLOCKED && r.remaining_in_burst == 0) {
                r.burst_idx++;
                if (r.burst_idx >= r.proc.bursts.size()) {
                    r.state = State::FINISHED;
                    r.completion = t;
                } else {
                    const auto& nb = r.proc.bursts[r.burst_idx];
                    r.remaining_in_burst = nb.duration;
                    if (nb.type == BurstType::CPU) {
                        r.state = State::READY;
                        r.ready_seq = ready_seq_counter++;
                    }
                }
            }
        }
    }

    int total_cpu_used = 0;
    int finished_count = 0;
    for (const auto& r : rts) {
        ProcessStats st;
        st.pid = r.proc.pid;
        st.arrival = r.proc.arrival;
        st.finished = (r.state == State::FINISHED);
        st.final_state = r.state;
        st.time_running = r.time_running;
        st.time_ready = r.time_ready;
        st.time_blocked = r.time_blocked;
        st.completion = st.finished ? r.completion : -1;
        st.turnaround = st.finished ? (r.completion - r.proc.arrival) : -1;
        st.waiting = r.time_ready;
        st.response = (r.first_run < 0) ? -1 : (r.first_run - r.proc.arrival);
        result.stats.push_back(st);
        total_cpu_used += r.time_running;
        if (st.finished) finished_count++;
    }

    if (finished_count > 0) {
        double sum_t = 0, sum_w = 0, sum_r = 0;
        for (const auto& s : result.stats) {
            if (!s.finished) continue;
            sum_t += s.turnaround;
            sum_w += s.waiting;
            sum_r += (s.response < 0 ? 0 : s.response);
        }
        result.avg_turnaround = sum_t / finished_count;
        result.avg_waiting = sum_w / finished_count;
        result.avg_response = sum_r / finished_count;
    } else {
        result.avg_turnaround = result.avg_waiting = result.avg_response = 0.0;
    }
    int total_time = static_cast<int>(result.gantt.size());
    result.cpu_utilization = (total_time > 0) ? (double)total_cpu_used / total_time : 0.0;
    result.total_time = total_time;
    result.finished_count = finished_count;
    result.total_count = static_cast<int>(rts.size());
    result.time_limit_reached = bounded && (t >= hard_limit) && any_alive(rts);

    return result;
}

}
