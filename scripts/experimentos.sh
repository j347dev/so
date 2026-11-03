#!/bin/bash
# roda todas as execucoes da secao 11 e guarda a saida e o gantt de cada uma em resultados/
make
mkdir -p resultados

for subst in lru fifo; do
  for cen in cenario_A_t0 cenario_B_espacadas; do
    for cpus in 1 4; do
      for sched in fcfs sjf rr2 rr5 rr10 rr20 rr50; do
        if [[ $sched == rr* ]]; then
          args="--sched rr --quantum ${sched#rr}"
        else
          args="--sched $sched"
        fi
        nome="${cen}_${sched}_${cpus}cpu_${subst}"
        ./simulador --arquivo cenarios/$cen.json --cpus $cpus --subst $subst $args > resultados/$nome.txt
        mv gantt.csv resultados/gantt_$nome.csv
        echo "$nome ok"
      done
    done
  done
done