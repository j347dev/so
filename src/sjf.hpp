//sjf.hpp (nao preemptivo)
//escolhe o pronto com a menor parte restante do burst de CPU atual.
//achar o minimo rápido
#pragma once
#include <set>
#include <tuple>
#include "escalonador.hpp"

class SJF : public Escalonador {
    struct Cmp {
        bool operator()(const Processo* a, const Processo* b) const {
            //o restante nao muda enquanto o processo esta na fila
            return std::make_tuple(a->restante(), a->seqFila) <
                   std::make_tuple(b->restante(), b->seqFila);
        }
    };
    std::set<Processo*, Cmp> fila;
public:
    std::string nome() const override { return "SJF"; }
    void inserir(Processo* p) override { fila.insert(p); }
    Processo* retirar() override {
        auto it = fila.begin();
        Processo* p = *it;
        fila.erase(it);
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
