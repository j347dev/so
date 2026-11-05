//main.cpp - roda a simulacao e mostra o resumo (argumentos estão no README.md)
//exemplo: ./simulador --arquivo cenarios/cenario_A_t0.json --cpus 4 --sched rr --quantum 10 --subst lru
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include "simulador.hpp"

int main(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--log") { cfg.log = true; continue; }
        if (i + 1 >= argc) { std::cout << "faltou o valor de " << a << "\n"; return 1; }
        std::string v = argv[++i];
        if (a == "--arquivo") cfg.arquivo = v;
        else if (a == "--cpus") cfg.cpus = std::stoi(v);
        else if (a == "--sched") cfg.sched = v;
        else if (a == "--quantum") cfg.quantum = std::stoi(v);
        else if (a == "--subst") cfg.subst = v;
        else if (a == "--quadros") cfg.quadros = std::stoi(v);
        else if (a == "--custo-troca") cfg.custoTroca = std::stoi(v);
        else if (a == "--custo-fault") cfg.custoFault = std::stoi(v);
        else { std::cout << "argumento invalido: " << a << "\n"; return 1; }
    }
    if (cfg.arquivo.empty()) {
        std::cout << "uso: ./simulador --arquivo <json> [--cpus 1|4] [--sched fcfs|sjf|rr] [--quantum n]\n"
                     "                 [--subst fifo|lru] [--quadros n] [--custo-troca n] [--log]\n";
        return 1;
    }
    if (cfg.sched != "fcfs" && cfg.sched != "sjf" && cfg.sched != "rr") { std::cout << "--sched deve ser fcfs, sjf ou rr\n"; return 1; }
    if (cfg.subst != "fifo" && cfg.subst != "lru") { std::cout << "--subst deve ser fifo ou lru\n"; return 1; }
    if (cfg.cpus < 1) { std::cout << "--cpus deve ser pelo menos 1\n"; return 1; }

    std::vector<Processo> procs;
    try {
        procs = carregarProcessos(cfg.arquivo);
    } catch (std::exception& e) {
        std::cout << "erro lendo o arquivo: " << e.what() << "\n";
        return 1;
    }

    Simulador sim(cfg, procs);
    Resultado r = sim.executar();

    //roda a mesma carga com a outra quantidade de CPUs (1 a 4)
    Config outra = cfg;
    outra.cpus = (cfg.cpus == 1) ? 4 : 1;
    outra.log = false;
    Simulador sim2(outra, procs);
    Resultado r2 = sim2.executar();
    int m1 = (cfg.cpus == 1) ? r.makespan : r2.makespan;
    int m4 = (cfg.cpus == 1) ? r2.makespan : r.makespan;
    double speedup = (double)m1 / m4;

    printf("PARAMETROS\n");
    printf("arquivo: %s | cpus: %d | sched: %s", cfg.arquivo.c_str(), cfg.cpus, cfg.sched.c_str());
    if (cfg.sched == "rr") printf(" | quantum: %d", cfg.quantum);
    printf("\nsubst: %s | quadros: %d | custo troca: %d\n", cfg.subst.c_str(), cfg.quadros, cfg.custoTroca);

    int nc = (cfg.cpus == 1) ? 4 : cfg.cpus; //numero de CPUs na comparacao
    printf("\nMETRICAS\n");
    printf("Espera media:      %.2f\n", r.esperaMedia);
    printf("Espera maxima:     %lld\n", r.esperaMax);
    printf("Turnaround medio:  %.2f\n", r.turnaroundMedio);
    printf("Resposta media:    %.2f\n", r.respostaMedia);
    printf("Makespan:          %d\n", r.makespan);
    printf("Speedup:           %.3f (M1=%d, M%d=%d)\n", speedup, m1, nc, m4);
    printf("Eficiencia:        %.3f\n", speedup / nc);
    printf("Trocas de contexto: %lld\n", r.trocasTotal);

    printf("\nPOR CPU\n");
    for (size_t c = 0; c < r.cpu.size(); c++) {
        EstatCPU& s = r.cpu[c];
        double M = r.makespan;
        printf("CPU %zu: util %lld (%.1f%%) | troca %lld (%.1f%%) | ocioso %lld (%.1f%%) | trocas %lld | utilizacao %.1f%%\n", c,
               s.util, 100 * s.util / M, s.troca, 100 * s.troca / M, s.ocioso, 100 * s.ocioso / M, s.trocas,
               100 * (s.util + s.troca) / M);
    }

    printf("\nMEMORIA\n");
    printf("Page faults:    %lld\n", r.faults);
    printf("Taxa de faults: %.2f%%\n", 100.0 * r.faults / r.referencias);

    //gantt de 0 a 200
    std::ofstream g("gantt.csv");
    g << "cpu,inicio,fim,evento\n";
    for (auto& s : r.gantt) g << s.cpu << "," << s.ini << "," << s.fim << "," << s.evento << "\n";
    printf("\nGantt salvo em gantt.csv\n");
    if (cfg.log) printf("Log salvo em log.txt (ate t=%d)\n", FIM_GANTT);
    return 0;
}