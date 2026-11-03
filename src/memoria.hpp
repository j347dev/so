// memoria.hpp - memoria fisica paginada, substituicao global FIFO ou LRU
//
// Estrutura:
//  - vector<Quadro> com N quadros globais; cada quadro guarda (dono, pagina),
//    o "carimbo" de carga (FIFO) e o de ultimo uso (LRU) e se esta protegido.
//  - pilha de quadros livres: achar quadro livre em O(1).
//  - cada processo tem quadroDaPagina[pagina] -> verificar acerto em O(1).
//  - vitima: varredura linear dos N quadros procurando o menor carimbo entre
//    os NAO protegidos. O(N) por fault, com N=256 e barato e simples de provar.
#pragma once
#include <string>
#include <vector>
#include "processo.hpp"

enum Politica { FIFO, LRU };

class Memoria {
    struct Quadro {
        Processo* dono = nullptr;
        int pagina = -1;
        long long carga = 0;   // ordem em que a pagina foi carregada (FIFO)
        long long uso = 0;     // ordem da ultima referencia (LRU)
        bool protegido = false;
    };
    std::vector<Quadro> quadros;
    std::vector<int> livres;
    Politica pol;
    long long relogio = 0;     // contador monotono: desempata eventos no mesmo instante
    int ocupados = 0;

public:
    Memoria(int n, Politica p) : quadros(n), pol(p) {
        for (int i = n - 1; i >= 0; --i) livres.push_back(i);
    }

    int totalQuadros() const { return (int)quadros.size(); }
    int quadrosOcupados() const { return ocupados; }

    bool presente(const Processo& p, int pag) const { return p.quadroDaPagina[pag] != -1; }

    // acerto: atualiza LRU e tira a protecao (a referencia do fault foi executada)
    void referenciar(Processo& p, int pag) {
        Quadro& q = quadros[p.quadroDaPagina[pag]];
        q.uso = ++relogio;
        q.protegido = false;
    }

    // tenta reservar um quadro para (p, pag). A pagina fica protegida ate o
    // processo executar a referencia. Retorna false se nao ha vitima possivel.
    bool carregar(Processo& p, int pag) {
        int idx = -1;
        if (!livres.empty()) {
            idx = livres.back();
            livres.pop_back();
            ++ocupados;
        } else {
            long long melhor = 0;
            for (int i = 0; i < (int)quadros.size(); ++i) {
                const Quadro& q = quadros[i];
                if (q.protegido) continue;
                long long chave = (pol == FIFO) ? q.carga : q.uso;
                if (idx == -1 || chave < melhor) { idx = i; melhor = chave; }
            }
            if (idx == -1) return false;  // todos protegidos
            Quadro& v = quadros[idx];
            v.dono->quadroDaPagina[v.pagina] = -1;  // vitima sai da memoria
        }
        Quadro& q = quadros[idx];
        q.dono = &p;
        q.pagina = pag;
        q.carga = q.uso = ++relogio;
        q.protegido = true;
        p.quadroDaPagina[pag] = idx;
        return true;
    }

    // termino: libera todos os quadros do processo
    void liberar(Processo& p) {
        for (int pag = 0; pag < p.paginas; ++pag) {
            int idx = p.quadroDaPagina[pag];
            if (idx == -1) continue;
            quadros[idx] = Quadro{};
            livres.push_back(idx);
            --ocupados;
            p.quadroDaPagina[pag] = -1;
        }
    }
};
