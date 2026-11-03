// fcfs.hpp - First Come, First Served
// Atende na ordem de entrada na fila de prontos. Fila simples (deque): O(1).
#pragma once
#include <deque>
#include "escalonador.hpp"

class FCFS : public Escalonador {
protected:
    std::deque<Processo*> fila;
public:
    std::string nome() const override { return "FCFS"; }
    void inserir(Processo* p) override { fila.push_back(p); }
    Processo* retirar() override {
        Processo* p = fila.front();
        fila.pop_front();
        return p;
    }
    bool vazia() const override { return fila.empty(); }
    size_t tamanho() const override { return fila.size(); }
    std::vector<int> listar() const override {
        std::vector<int> v;
        for (auto* p : fila) v.push_back(p->pid);
        return v;
    }
};
