# Simulador de escalonamento - Trabalho Sistemas Operacionais

Simulador de FCFS, SJF não preemptivo e Round Robin com 1 ou 4 CPUs, fila de prontos
única, E/S, custo de troca de contexto e memória paginada com substituição FIFO ou LRU.

## Compilar e rodar

```bash
make
./simulador --arquivo cenarios/cenario_B_espacadas.json --cpus 4 --sched rr --quantum 10 --subst lru
```

Argumentos: `--arquivo`, `--cpus` (1 ou 4), `--sched` (fcfs, sjf, rr), `--quantum`,
`--subst` (fifo, lru), `--quadros` (padrão 256), `--custo-troca` (padrão 1) e `--log`.
Também tem `--custo-fault` (padrão 20), que só usamos para rodar o exemplo 7.2 do enunciado,
onde o fault custa 5.

Cada execução mostra o resumo na tela e grava `gantt.csv`. Com `--log`,
grava também `log.txt` com os eventos, a fila de prontos, os bloqueados, as CPUs e os quadros
ocupados a cada instante em que algo muda (até t=200, porque o log inteiro de 1000 processos
passa de 400 MB). O speedup é calculado rodando a mesma carga com a outra quantidade de CPUs.

## Experimentos

```bash
bash scripts/experimentos.sh #roda as 28 execuções base em LRU e as mesmas 28 com FIFO
pip install matplotlib #biblioteca python para visualização de dados
python3 scripts/graficos.py #tabela_resumo.csv e gráficos em resultados/figuras/
```

Quantums do RR: 2, 5, 10, 20 e 50.

## Exemplos do enunciado (para conferir)

```bash
./simulador --arquivo testes/ex71_sem_memoria.json # espera média 4.67, makespan 13
./simulador --arquivo testes/ex72_faults.json --custo-troca 0 --custo-fault 5 # turnaround médio 10, makespan 13
./simulador --arquivo testes/ex73_fila.json --custo-troca 0 --sched fcfs # espera média 3.67
./simulador --arquivo testes/ex73_fila.json --custo-troca 0 --sched sjf # espera média 2.33
./simulador --arquivo testes/ex55_processo17.json --custo-troca 0 # turnaround 99, 4 faults
```

## Arquivos

- `src/processo.hpp`: struct do processo e leitura do JSON
- `src/escalonador.hpp`, `fcfs.hpp`, `sjf.hpp`, `rr.hpp`: os escalonadores
- `src/memoria.hpp`: quadros, page fault e escolha da vítima (no caso FIFO ou LRU)
- `src/simulador.hpp`: o laço da simulação
- `src/main.cpp`: argumentos e resumo

## Decisões que tomamos
- A troca de contexto é cobrada quando a CPU recebe um processo diferente do último que ela executou.
- No page fault, o quadro é reservado na hora e a página fica protegida até o processo usar ela.
- Se todos os quadros estão protegidos, o processo espera 20 e tenta de novo; se conseguir, espera mais 20 pra página chegar. O fault conta uma vez só.
- Quando duas CPUs estouram o quantum ao mesmo tempo, a de menor índice é tratada primeiro.

## Limitações
- O SJF usa o tamanho real do burst; num SO de verdade isso teria que ser estimado.
- Não tem fila de disco, então E/S e faults acontecem todos em paralelo.
- Troca de contexto com custo fixo, sem considerar cache.
- O log vai só até t=200, porque a versão completa passa de 400 MB.