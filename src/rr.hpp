// rr.hpp - Round Robin
// Mesma fila circular do FCFS; a diferenca e o quantum, que o simulador usa
// para preemptar o processo (que volta para o fim da fila).
#pragma once
#include "fcfs.hpp"

class RR : public FCFS {
    int q;
public:
    explicit RR(int quantum) : q(quantum) {}
    std::string nome() const override { return "RR"; }
    int quantum() const override { return q; }
};
