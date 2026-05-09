#include "scheduler.hpp"
#include <cassert>
#include <iostream>

using namespace sched;

static const char* st(State s) {
    switch (s) {
        case State::NEW: return "_";
        case State::READY: return "P";
        case State::RUNNING: return "R";
        case State::BLOCKED: return "B";
        case State::FINISHED: return "F";
    }
    return "?";
}

void print(const std::string& title, const SimulationResult& r) {
    std::cout << "\n=== " << title << " ===\n";
    std::cout << "Tempo total: " << r.gantt.size()
              << "  | TA-medio: " << r.avg_turnaround
              << "  | Espera-medio: " << r.avg_waiting
              << "  | Resp-medio: " << r.avg_response
              << "  | CPU%: " << (r.cpu_utilization * 100) << "\n";
    std::cout << "Gantt: ";
    for (int p : r.gantt) std::cout << (p < 0 ? "_" : std::to_string(p)) << " ";
    std::cout << "\n";
    for (const auto& s : r.stats) {
        std::cout << "  P" << s.pid
                  << " arrival=" << s.arrival
                  << " completion=" << s.completion
                  << " TA=" << s.turnaround
                  << " W=" << s.waiting
                  << " R=" << s.response << "\n";
    }
}

int main() {
    // Caso classico FCFS: P1(arr=0, burst=5), P2(arr=1, burst=3), P3(arr=2, burst=8)
    {
        std::vector<Process> ps = {
            {1, 0, 1, {{BurstType::CPU, 5}}},
            {2, 1, 1, {{BurstType::CPU, 3}}},
            {3, 2, 1, {{BurstType::CPU, 8}}},
        };
        auto r = run(ps, {Algorithm::FCFS, false, 0});
        print("FCFS (sem IO)", r);
        // Esperado: P1 0-5, P2 5-8, P3 8-16
        assert(r.stats[0].completion == 5);
        assert(r.stats[1].completion == 8);
        assert(r.stats[2].completion == 16);
    }

    // SJF nao preemptivo
    {
        std::vector<Process> ps = {
            {1, 0, 1, {{BurstType::CPU, 7}}},
            {2, 2, 1, {{BurstType::CPU, 4}}},
            {3, 4, 1, {{BurstType::CPU, 1}}},
            {4, 5, 1, {{BurstType::CPU, 4}}},
        };
        auto r = run(ps, {Algorithm::SJF, false, 0});
        print("SJF nao-preemptivo", r);
        // P1 roda 0-7, depois pode escolher P2 (burst=4), P3 (1), P4 (4 mas chega t=5)
        // No t=7 disponiveis: P2 (4), P3 (1), P4 (4) — escolhe P3
        // Depois t=8: P2 e P4 (ambos 4) — desempate por pid → P2
        // P2 8-12, P4 12-16
        assert(r.stats[0].completion == 7);   // P1
        assert(r.stats[2].completion == 8);   // P3
        assert(r.stats[1].completion == 12);  // P2
        assert(r.stats[3].completion == 16);  // P4
    }

    // SJF preemptivo (SRTF)
    {
        std::vector<Process> ps = {
            {1, 0, 1, {{BurstType::CPU, 8}}},
            {2, 1, 1, {{BurstType::CPU, 4}}},
            {3, 2, 1, {{BurstType::CPU, 9}}},
            {4, 3, 1, {{BurstType::CPU, 5}}},
        };
        auto r = run(ps, {Algorithm::SJF, true, 0});
        print("SJF preemptivo (SRTF)", r);
        // P1 t0-1 (rem=7), P2 chega rem=4, preempta. P2 t1-5 (rem=0)
        // t=5: P1 rem=7, P3 rem=9, P4 rem=5 → P4 (ou P1 com tie?). P4=5, P1=7. P4 vence.
        // P4 t5-10. t10: P1 rem=7, P3 rem=9 → P1
        // P1 t10-17. P3 t17-26.
        assert(r.stats[1].completion == 5);   // P2 termina cedo
        assert(r.stats[3].completion == 10);  // P4
        assert(r.stats[0].completion == 17);  // P1
        assert(r.stats[2].completion == 26);  // P3
    }

    // Round Robin
    {
        std::vector<Process> ps = {
            {1, 0, 1, {{BurstType::CPU, 5}}},
            {2, 1, 1, {{BurstType::CPU, 3}}},
            {3, 2, 1, {{BurstType::CPU, 4}}},
        };
        auto r = run(ps, {Algorithm::ROUND_ROBIN, false, 2});
        print("Round Robin (q=2)", r);
        // P1[0-2], P2[2-4], P1[4-6], P3[6-8], P2[8-9], P1[9-10], P3[10-12]
        // P2 termina em 9, P1 em 10, P3 em 12
        assert(r.stats[1].completion == 9);
        assert(r.stats[0].completion == 10);
        assert(r.stats[2].completion == 12);
    }

    // Prioridade nao preemptiva
    {
        std::vector<Process> ps = {
            {1, 0, 3, {{BurstType::CPU, 4}}},
            {2, 1, 1, {{BurstType::CPU, 3}}},
            {3, 2, 2, {{BurstType::CPU, 2}}},
        };
        auto r = run(ps, {Algorithm::PRIORITY, false, 0});
        print("Prioridade nao-preemptiva", r);
        // P1 roda 0-4 (mesmo chegando processos com prio menor). t=4: P2 (1) e P3 (2) → P2.
        // P2 4-7. P3 7-9.
        assert(r.stats[0].completion == 4);
        assert(r.stats[1].completion == 7);
        assert(r.stats[2].completion == 9);
    }

    // Prioridade preemptiva
    {
        std::vector<Process> ps = {
            {1, 0, 3, {{BurstType::CPU, 4}}},
            {2, 1, 1, {{BurstType::CPU, 3}}},
            {3, 2, 2, {{BurstType::CPU, 2}}},
        };
        auto r = run(ps, {Algorithm::PRIORITY, true, 0});
        print("Prioridade preemptiva", r);
        // P1 t0-1. t=1: P2 prio=1 < 3, preempta. P2 1-4. t=2 P3 prio=2 chega mas P2 prio=1 < 2, segue.
        // t=4: P1 (prio=3, rem=3), P3 (prio=2, rem=2) → P3
        // P3 4-6. P1 6-9.
        assert(r.stats[1].completion == 4);
        assert(r.stats[2].completion == 6);
        assert(r.stats[0].completion == 9);
    }

    // Com I/O
    {
        std::vector<Process> ps = {
            {1, 0, 1, {{BurstType::CPU, 3}, {BurstType::IO, 2}, {BurstType::CPU, 2}}},
            {2, 1, 1, {{BurstType::CPU, 4}}},
        };
        auto r = run(ps, {Algorithm::FCFS, false, 0});
        print("FCFS com IO", r);
        // P1 roda 0-3, vai pra IO. t=3: P2 ready, roda 3-7.
        // P1 IO de 3 ate 5. t=5 P1 ready. Mas P2 esta running ate 7 (FCFS nao preempta).
        // P1 7-9.
        assert(r.stats[1].completion == 7);
        assert(r.stats[0].completion == 9);
    }

    // Limite de tempo: simulacao parada antes do fim
    {
        std::vector<Process> ps = {
            {1, 0, 1, {{BurstType::CPU, 5}}},
            {2, 1, 1, {{BurstType::CPU, 5}}},
            {3, 2, 1, {{BurstType::CPU, 5}}},
            {4, 100, 1, {{BurstType::CPU, 5}}},  // chega depois do limite
        };
        Config cfg{Algorithm::FCFS, false, 0};
        cfg.max_time = 8;
        auto r = run(ps, cfg);
        print("FCFS bounded (max_time=8)", r);
        // P1 termina em 5, P2 vai ate 8 (rodou 3 unidades de 5), P3 nao executou, P4 nao chegou.
        assert(r.total_time == 8);
        assert(r.time_limit_reached);
        assert(r.finished_count == 1);          // so P1
        assert(r.stats[0].finished == true);
        assert(r.stats[0].completion == 5);
        assert(r.stats[1].finished == false);
        assert(r.stats[1].final_state == State::RUNNING);
        assert(r.stats[1].time_running == 3);   // rodou 3 das 5 unidades
        assert(r.stats[1].response == 0);
        assert(r.stats[2].finished == false);
        assert(r.stats[2].final_state == State::READY);
        assert(r.stats[2].time_running == 0);   // nunca executou
        assert(r.stats[2].response == -1);      // nunca executou
        assert(r.stats[3].finished == false);
        assert(r.stats[3].final_state == State::NEW);  // nao chegou ainda
        assert(r.stats[3].time_running == 0);
    }

    // Sem limite (max_time=0): comportamento padrao
    {
        std::vector<Process> ps = {
            {1, 0, 1, {{BurstType::CPU, 3}}},
            {2, 1, 1, {{BurstType::CPU, 2}}},
        };
        Config cfg{Algorithm::FCFS, false, 0};
        cfg.max_time = 0;
        auto r = run(ps, cfg);
        assert(!r.time_limit_reached);
        assert(r.finished_count == 2);
        assert(r.total_time == 5);
    }

    std::cout << "\n[OK] Todos os testes passaram.\n";
    return 0;
}
