// escalonador.hpp - interface comum da fila de prontos
// Cada politica (FCFS, SJF, RR) so decide a ORDEM da fila de prontos.
// O resto (CPUs, tempo, E/S, memoria) fica no simulador, igual para todas.
#pragma once
#include <string>
#include <vector>
#include "processo.hpp"

class Escalonador {
public:
    virtual ~Escalonador() = default;
    virtual std::string nome() const = 0;
    virtual void inserir(Processo* p) = 0;     // p->seqFila ja definido pelo simulador
    virtual Processo* retirar() = 0;           // proximo a ser despachado
    virtual bool vazia() const = 0;
    virtual size_t tamanho() const = 0;
    virtual std::vector<int> listar() const = 0;  // pids na ordem em que seriam atendidos
    virtual int quantum() const { return 0; }     // 0 = nao preemptivo
};
