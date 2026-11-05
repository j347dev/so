//rr.hpp
//quantum para preemptar o processo
#pragma once
#include "fcfs.hpp"

//using namespace std; 

//vai herdar tudo de FCFS, e FCFS herdou de escalonador.hpp, a diferença vai ser no quantum
class RR : public FCFS {
    int q;
public:
    explicit RR(int quantum) : q(quantum) {}
    std::string nome() const override { return "RR"; }
    int quantum() const override { return q; }
};
