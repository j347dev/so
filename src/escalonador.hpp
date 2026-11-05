//escalonador.hpp - interface comum da fila de prontos
#pragma once //usado para o compilador
#include <string>
#include <vector>
#include "processo.hpp"

//relacionado a objeto, é uma classe abstrata que funciona como interface
//cada escalonador vai só decidir a ordem da fila
class Escalonador {
public:
    virtual ~Escalonador() = default;
    virtual std::string nome() const = 0;
    virtual void inserir(Processo* p) = 0; //p->seqFila ja definido pelo simulador
    virtual Processo* retirar() = 0; //proximo a ser despachado
    virtual bool vazia() const = 0;
    virtual size_t tamanho() const = 0;
    virtual std::vector<int> listar() const = 0; //pids na ordem em que seriam atendidos
    virtual int quantum() const { return 0; } //0 = nao preemptivo
};
