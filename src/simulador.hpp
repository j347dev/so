// simulador.hpp
// Simulacao em tempo discreto. A cada instante t:
//   1) termina o que aconteceu em [t-1, t): fim da troca, unidade executada,
//      fim de burst (vai pra E/S ou termina), fim do quantum
//   2) chegadas e retornos de E/S / page fault entram na fila (ordem de pid)
//   3) quem estourou o quantum vai pro fim da fila
//   4) despacho (CPU de menor indice primeiro) e execucao da unidade [t, t+1)
#pragma once
#include <fstream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <tuple>
#include <vector>
#include "escalonador.hpp"
#include "fcfs.hpp"
#include "memoria.hpp"
#include "processo.hpp"
#include "rr.hpp"
#include "sjf.hpp"

const int FIM_GANTT = 200;   // janela do Gantt e do log

struct Config {
    std::string arquivo;
    int cpus = 1;
    std::string sched = "fcfs";
    int quantum = 10;
    std::string subst = "lru";
    int quadros = 256;
    int custoTroca = 1;
    int custoFault = 20;
    bool log = false;
};

struct Segmento { int cpu, ini, fim; std::string evento; };

struct EstatCPU { long long util = 0, troca = 0, ocioso = 0, trocas = 0; };

struct Resultado {
    int makespan = 0;
    double esperaMedia = 0, turnaroundMedio = 0, respostaMedia = 0;
    long long esperaMax = 0;
    long long trocasTotal = 0, faults = 0, referencias = 0;
    std::vector<EstatCPU> cpu;
    std::vector<Segmento> gantt;
};

class Simulador {
    // tipos de evento
    enum { CHEGADA, RETORNO_ES, RETORNO_FAULT, TENTA_DE_NOVO };
    // estados da CPU
    enum { LIVRE, TROCA, COM_PROC, EXEC };

    struct CPU {
        int estado = LIVRE;
        Processo* p = nullptr;
        int fimTroca = 0;
        int quantumUsado = 0;
        bool estourou = false;
        int ultimoPid = -1;
        EstatCPU st;
    };

    Config cfg;
    std::vector<Processo>& procs;
    std::unique_ptr<Escalonador> esc;
    Memoria mem;
    std::vector<CPU> cpus;

    // eventos futuros: (instante, pid, tipo, indice no vetor)
    // a fila de prioridade ja deixa ordenado por instante e depois por pid
    std::priority_queue<std::tuple<int, int, int, int>,
                        std::vector<std::tuple<int, int, int, int>>,
                        std::greater<std::tuple<int, int, int, int>>> eventos;
    std::map<int, std::pair<int, std::string>> bloqueados;  // so pro log
    long long seq = 0;
    int terminados = 0;
    Resultado res;

    std::ofstream logf;
    std::vector<std::string> evAgora;

    void ev(int t, const std::string& s) {
        if (cfg.log && t <= FIM_GANTT) evAgora.push_back(s);
    }

    void gantt(int c, int ini, int fim, const std::string& e) {
        if (ini >= FIM_GANTT) return;
        if (fim > FIM_GANTT) fim = FIM_GANTT;
        // se for continuacao do ultimo pedaco dessa CPU, so estende
        for (int i = (int)res.gantt.size() - 1; i >= 0; i--) {
            if (res.gantt[i].cpu != c) continue;
            if (res.gantt[i].fim == ini && res.gantt[i].evento == e) {
                res.gantt[i].fim = fim;
                return;
            }
            break;
        }
        res.gantt.push_back({c, ini, fim, e});
    }

    void entrarFila(Processo* p, int t) {
        p->entradaFila = t;
        p->seqFila = ++seq;
        esc->inserir(p);
    }

    void bloquear(Processo* p, int ate, int tipo, const std::string& motivo) {
        eventos.emplace(ate, p->pid, tipo, (int)(p - procs.data()));
        bloqueados[p->pid] = {ate, motivo};
    }

    void tratarFault(Processo* p, int t) {
        if (mem.carregar(*p, p->proximaRef())) {
            bloquear(p, t + cfg.custoFault, RETORNO_FAULT, "fault");
        } else {
            // todos os quadros protegidos: espera e tenta de novo
            bloquear(p, t + cfg.custoFault, TENTA_DE_NOVO, "sem vitima");
            ev(t, "SEM VITIMA P" + std::to_string(p->pid));
        }
    }

    void escreverLog(int t) {
        if (evAgora.empty()) return;
        logf << "t=" << t << "\n  eventos:";
        for (auto& s : evAgora) logf << " [" << s << "]";
        logf << "\n  prontos:";
        for (int pid : esc->listar()) logf << " P" << pid;
        logf << "\n  bloqueados:";
        for (auto& b : bloqueados) logf << " P" << b.first << "(" << b.second.second << " ate " << b.second.first << ")";
        logf << "\n  cpus:";
        for (size_t c = 0; c < cpus.size(); c++) {
            logf << " CPU" << c << "=";
            if (cpus[c].estado == LIVRE) logf << "OCIOSA";
            else if (cpus[c].estado == TROCA) logf << "TROCA(P" << cpus[c].p->pid << ")";
            else logf << "P" << cpus[c].p->pid;
        }
        logf << "\n  quadros ocupados: " << mem.quadrosOcupados() << "/" << cfg.quadros << "\n";
        evAgora.clear();
    }

public:
    Simulador(const Config& c, std::vector<Processo>& ps)
        : cfg(c), procs(ps), mem(c.quadros, c.subst == "fifo" ? FIFO : LRU), cpus(c.cpus) {
        if (c.sched == "fcfs") esc = std::make_unique<FCFS>();
        else if (c.sched == "sjf") esc = std::make_unique<SJF>();
        else esc = std::make_unique<RR>(c.quantum);
        if (c.log) logf.open("log.txt");
    }

    Resultado executar() {
        for (auto& p : procs) reiniciar(p);
        for (size_t i = 0; i < procs.size(); i++)
            eventos.emplace(procs[i].chegada, procs[i].pid, CHEGADA, (int)i);

        int n = procs.size();
        int Q = esc->quantum();
        int t = 0;

        while (true) {
            // 1) fecha o intervalo [t-1, t)
            for (size_t c = 0; c < cpus.size(); c++) {
                CPU& k = cpus[c];
                if (k.estado == TROCA && k.fimTroca == t) {
                    k.estado = COM_PROC;
                } else if (k.estado == EXEC) {
                    Processo* p = k.p;
                    p->unidade++;
                    k.quantumUsado++;
                    k.estado = COM_PROC;
                    if (p->unidade == p->bursts[p->burstAtual].duracao) {
                        // acabou o burst de CPU
                        p->burstAtual++;
                        p->unidade = 0;
                        k.p = nullptr;
                        k.estado = LIVRE;
                        if (p->burstAtual == p->bursts.size()) {
                            p->termino = t;
                            mem.liberar(*p);
                            terminados++;
                            ev(t, "TERMINO P" + std::to_string(p->pid));
                        } else {
                            int es = p->bursts[p->burstAtual].duracao;  // o proximo eh E/S
                            p->burstAtual++;                             // e depois dele CPU de novo
                            bloquear(p, t + es, RETORNO_ES, "E/S");
                            ev(t, "FIM BURST P" + std::to_string(p->pid) + " (E/S ate " + std::to_string(t + es) + ")");
                        }
                    } else if (Q > 0 && k.quantumUsado >= Q) {
                        k.estourou = true;
                    }
                }
            }
            if (terminados == n) break;

            // 2) chegadas e retornos
            while (!eventos.empty() && std::get<0>(eventos.top()) == t) {
                auto [te, pid, tipo, idx] = eventos.top();
                eventos.pop();
                Processo* p = &procs[idx];
                bloqueados.erase(pid);
                if (tipo == TENTA_DE_NOVO) {
                    tratarFault(p, t);
                    continue;
                }
                entrarFila(p, t);
                if (tipo == CHEGADA) ev(t, "CHEGADA P" + std::to_string(pid));
                else if (tipo == RETORNO_ES) ev(t, "RETORNO E/S P" + std::to_string(pid));
                else ev(t, "RETORNO FAULT P" + std::to_string(pid));
            }

            // 3) fim do quantum
            for (size_t c = 0; c < cpus.size(); c++) {
                CPU& k = cpus[c];
                if (!k.estourou) continue;
                k.estourou = false;
                if (!esc->vazia()) {
                    ev(t, "PREEMPCAO P" + std::to_string(k.p->pid));
                    entrarFila(k.p, t);
                    k.p = nullptr;
                    k.estado = LIVRE;
                } else {
                    k.quantumUsado = 0;  // ninguem esperando, continua com quantum novo
                }
            }

            // 4) despacho e execucao
            for (size_t c = 0; c < cpus.size(); c++) {
                CPU& k = cpus[c];
                while (true) {
                    if (k.estado == LIVRE) {
                        if (esc->vazia()) break;
                        Processo* p = esc->retirar();
                        p->espera += t - p->entradaFila;
                        if (p->primeiroDespacho < 0) p->primeiroDespacho = t;
                        k.p = p;
                        k.quantumUsado = 0;
                        bool cobra = p->pid != k.ultimoPid && cfg.custoTroca > 0;
                        k.ultimoPid = p->pid;
                        ev(t, "DESPACHO P" + std::to_string(p->pid) + " CPU" + std::to_string(c));
                        if (cobra) {
                            k.estado = TROCA;
                            k.fimTroca = t + cfg.custoTroca;
                            k.st.trocas++;
                            break;
                        }
                        k.estado = COM_PROC;
                    }
                    if (k.estado != COM_PROC) break;

                    Processo* p = k.p;
                    int pag = p->proximaRef();
                    if (pag < 0 || mem.presente(*p, pag)) {
                        // acerto: executa essa unidade
                        if (pag >= 0) mem.referenciar(*p, pag);
                        res.referencias++;
                        k.estado = EXEC;
                        break;
                    }
                    // page fault: nao gasta a unidade e a CPU ja fica livre
                    res.faults++;
                    ev(t, "FAULT P" + std::to_string(p->pid) + " pag " + std::to_string(pag));
                    k.p = nullptr;
                    k.estado = LIVRE;
                    tratarFault(p, t);
                }
            }

            if (cfg.log) escreverLog(t);

            // se nao tem nada pra fazer, pula direto pro proximo evento
            bool parado = esc->vazia();
            for (auto& k : cpus) if (k.estado != LIVRE) parado = false;
            int prox = t + 1;
            if (parado) prox = std::get<0>(eventos.top());

            for (size_t c = 0; c < cpus.size(); c++) {
                CPU& k = cpus[c];
                if (k.estado == EXEC) {
                    k.st.util += prox - t;
                    gantt(c, t, prox, "P" + std::to_string(k.p->pid));
                } else if (k.estado == TROCA) {
                    k.st.troca += prox - t;
                    gantt(c, t, prox, "TROCA");
                } else {
                    gantt(c, t, prox, "OCIOSA");
                }
            }
            t = prox;
        }

        // metricas
        res.makespan = t;
        double somaEspera = 0, somaTat = 0, somaResp = 0;
        for (auto& p : procs) {
            somaEspera += p.espera;
            somaTat += p.termino - p.chegada;
            somaResp += p.primeiroDespacho - p.chegada;
            if (p.espera > res.esperaMax) res.esperaMax = p.espera;
        }
        res.esperaMedia = somaEspera / n;
        res.turnaroundMedio = somaTat / n;
        res.respostaMedia = somaResp / n;
        for (auto& k : cpus) {
            k.st.ocioso = res.makespan - k.st.util - k.st.troca;
            res.trocasTotal += k.st.trocas;
            res.cpu.push_back(k.st);
        }
        return res;
    }
};