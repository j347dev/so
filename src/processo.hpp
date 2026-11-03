// processo.hpp - estrutura de um processo e leitura do JSON de carga
#pragma once
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Uma etapa da vida do processo: burst de CPU (com refs) ou de E/S
struct Burst {
    bool ehCpu = true;
    int duracao = 0;           // unidades de tempo (cpu ou es)
    std::vector<int> refs;     // so para CPU: pagina acessada em cada unidade
};

struct Processo {
    // ---- dados do arquivo (carga) ----
    int pid = 0;
    int chegada = 0;
    std::string classe;        // "curto"/"longo": SO para analise, o escalonador NAO usa
    int paginas = 0;
    std::vector<Burst> bursts;

    // ---- estado da simulacao ----
    size_t burstAtual = 0;     // indice em bursts
    int unidade = 0;           // unidades ja executadas do burst de CPU atual
    long long seqFila = 0;     // ordem de entrada na fila de prontos (desempate)
    int entradaFila = 0;       // instante em que entrou na fila (para medir espera)
    long long espera = 0;      // soma dos intervalos na fila de prontos
    int primeiroDespacho = -1;
    int termino = -1;
    int faults = 0;
    std::vector<int> quadroDaPagina;  // pagina -> indice do quadro (-1 = fora da memoria)

    // parte que falta do burst de CPU atual (usado pelo SJF)
    int restante() const { return bursts[burstAtual].duracao - unidade; }
    // pagina referenciada pela proxima unidade de CPU (-1 se o burst nao tem refs)
    int proximaRef() const {
        const Burst& b = bursts[burstAtual];
        return b.refs.empty() ? -1 : b.refs[unidade];
    }
};

inline std::vector<Processo> carregarProcessos(const std::string& caminho) {
    std::ifstream in(caminho);
    if (!in) throw std::runtime_error("nao foi possivel abrir " + caminho);
    nlohmann::json j;
    in >> j;
    std::vector<Processo> v;
    v.reserve(j.size());
    for (const auto& jp : j) {
        Processo p;
        p.pid = jp.at("pid").get<int>();
        p.chegada = jp.at("chegada").get<int>();
        p.classe = jp.value("classe", std::string("?"));
        p.paginas = jp.value("paginas", 0);
        for (const auto& jb : jp.at("bursts")) {
            Burst b;
            if (jb.contains("cpu")) {
                b.ehCpu = true;
                b.duracao = jb["cpu"].get<int>();
                if (jb.contains("refs")) b.refs = jb["refs"].get<std::vector<int>>();
                if (!b.refs.empty() && (int)b.refs.size() != b.duracao)
                    throw std::runtime_error("pid " + std::to_string(p.pid) + ": refs != cpu");
                for (int r : b.refs)
                    if (r < 0 || r >= p.paginas)
                        throw std::runtime_error("pid " + std::to_string(p.pid) + ": ref fora de [0,paginas)");
            } else {
                b.ehCpu = false;
                b.duracao = jb.at("es").get<int>();
            }
            p.bursts.push_back(std::move(b));
        }
        if (p.bursts.empty() || !p.bursts.front().ehCpu || !p.bursts.back().ehCpu)
            throw std::runtime_error("pid " + std::to_string(p.pid) + ": deve comecar e terminar com CPU");
        p.quadroDaPagina.assign(p.paginas, -1);
        v.push_back(std::move(p));
    }
    return v;
}

// volta o processo ao estado inicial (permite rodar a mesma carga varias vezes)
inline void reiniciar(Processo& p) {
    p.burstAtual = 0; p.unidade = 0; p.seqFila = 0; p.entradaFila = 0;
    p.espera = 0; p.primeiroDespacho = -1; p.termino = -1; p.faults = 0;
    p.quadroDaPagina.assign(p.paginas, -1);
}
