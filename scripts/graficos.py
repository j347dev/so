#le as saidas em resultados/*.txt e gera a tabela resumo e os graficos
#precisa de: pip install matplotlib
import glob, os, re
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

os.makedirs("resultados/figuras", exist_ok=True)

def numero(texto, nome):
    return float(re.search(nome + r":\s+([\d.]+)", texto).group(1))

#carrega todas as execucoes
dados = []
for arq in glob.glob("resultados/*.txt"):
    cen, sched, cpus, subst = re.match(r"(cenario_[AB]_\w+?)_(\w+?)_(\d)cpu_(\w+)\.txt", os.path.basename(arq)).groups()
    t = open(arq).read()
    dados.append({
        "cenario": cen, "sched": sched, "cpus": int(cpus), "subst": subst,
        "espera": numero(t, "Espera media"), "espera_max": numero(t, "Espera maxima"),
        "turnaround": numero(t, "Turnaround medio"), "resposta": numero(t, "Resposta media"),
        "makespan": numero(t, "Makespan"), "speedup": numero(t, "Speedup"),
        "eficiencia": numero(t, "Eficiencia"), "trocas": numero(t, "Trocas de contexto"),
        "faults": numero(t, "Page faults"), "taxa": numero(t, "Taxa de faults"),
    })

ordem = ["fcfs", "sjf", "rr2", "rr5", "rr10", "rr20", "rr50"]
def nome(s):
    return "RR q=" + s[2:] if s.startswith("rr") else s.upper()
def pega(cen, cpus, subst="lru"):
    d = [x for x in dados if x["cenario"] == cen and x["cpus"] == cpus and x["subst"] == subst]
    return sorted(d, key=lambda x: ordem.index(x["sched"]))

cenarios = ["cenario_A_t0", "cenario_B_espacadas"]

#tabela resumo (LRU, 256 quadros), uma por cenario
with open("resultados/tabela_resumo.csv", "w") as f:
    f.write("cenario,cpus,escalonador,espera_media,espera_max,turnaround,resposta,makespan,speedup,eficiencia,trocas,page_faults,taxa_faults\n")
    for cen in cenarios:
        for cpus in [1, 4]:
            for x in pega(cen, cpus):
                f.write(f"{cen},{cpus},{nome(x['sched'])},{x['espera']},{x['espera_max']:.0f},{x['turnaround']},{x['resposta']},"
                        f"{x['makespan']:.0f},{x['speedup']},{x['eficiencia']},{x['trocas']:.0f},{x['faults']:.0f},{x['taxa']}\n")

#grafico de barras: uma metrica por escalonador, 1 x 4 CPUs
def barras(metrica, titulo, arquivo):
    fig, eixos = plt.subplots(1, 2, figsize=(12, 4))
    for ax, cen in zip(eixos, cenarios):
        for i, cpus in enumerate([1, 4]):
            d = pega(cen, cpus)
            ax.bar([j + i * 0.4 - 0.2 for j in range(len(d))], [x[metrica] for x in d], 0.4, label=f"{cpus} CPU")
        ax.set_xticks(range(len(ordem)))
        ax.set_xticklabels([nome(s) for s in ordem], rotation=30)
        ax.set_title(cen)
        ax.legend()
    fig.suptitle(titulo)
    fig.tight_layout()
    fig.savefig("resultados/figuras/" + arquivo, dpi=150)

barras("espera", "Espera media por escalonador (LRU, 256 quadros)", "espera_media.png")
barras("turnaround", "Turnaround medio por escalonador", "turnaround.png")

#speedup (so precisa das execucoes com 4 CPUs)
fig, ax = plt.subplots(figsize=(9, 4))
for i, cen in enumerate(cenarios):
    d = pega(cen, 4)
    ax.bar([j + i * 0.4 - 0.2 for j in range(len(d))], [x["speedup"] for x in d], 0.4, label=cen)
ax.axhline(4, color="black", linestyle="--", label="ideal")
ax.set_xticks(range(len(ordem)))
ax.set_xticklabels([nome(s) for s in ordem], rotation=30)
ax.set_title("Speedup por escalonador")
ax.legend()
fig.tight_layout()
fig.savefig("resultados/figuras/speedup.png", dpi=150)

#efeito do quantum no RR: espera, resposta e trocas
qs = [2, 5, 10, 20, 50]
fig, eixos = plt.subplots(1, 3, figsize=(15, 4))
for ax, (metrica, titulo) in zip(eixos, [("espera", "Espera media"), ("resposta", "Resposta media"), ("trocas", "Trocas de contexto")]):
    for cen in cenarios:
        for cpus in [1, 4]:
            d = {x["sched"]: x for x in pega(cen, cpus)}
            ax.plot(qs, [d[f"rr{q}"][metrica] for q in qs], marker="o", label=f"{cen[8]} / {cpus} CPU")
    ax.set_xlabel("quantum")
    ax.set_title(titulo)
    ax.legend(fontsize=8)
fig.suptitle("Efeito do quantum no RR")
fig.tight_layout()
fig.savefig("resultados/figuras/quantum_rr.png", dpi=150)

#FIFO x LRU: page faults
fig, eixos = plt.subplots(1, 2, figsize=(13, 4))
for ax, cen in zip(eixos, cenarios):
    rotulos = [f"{nome(s)}/{c}" for c in [1, 4] for s in ordem]
    for i, subst in enumerate(["fifo", "lru"]):
        vals = [x["faults"] for c in [1, 4] for x in pega(cen, c, subst)]
        ax.bar([j + i * 0.4 - 0.2 for j in range(len(vals))], vals, 0.4, label=subst.upper())
    ax.set_xticks(range(len(rotulos)))
    ax.set_xticklabels(rotulos, rotation=70, fontsize=7)
    ax.set_title(cen + " (escalonador/CPUs)")
    ax.legend()
fig.suptitle("Page faults: FIFO x LRU")
fig.tight_layout()
fig.savefig("resultados/figuras/fifo_x_lru.png", dpi=150)

# figura do gantt de cada execucao base (LRU)
for arq in glob.glob("resultados/gantt_*_lru.csv"):
    linhas = [l.strip().split(",") for l in open(arq).readlines()[1:]]
    ncpus = max(int(l[0]) for l in linhas) + 1
    fig, ax = plt.subplots(figsize=(18, 1 + 0.8 * ncpus))
    for cpu, ini, fim, ev in linhas:
        cpu, ini, fim = int(cpu), int(ini), int(fim)
        y = ncpus - 1 - cpu
        if ev == "TROCA":
            ax.barh(y, fim - ini, left=ini, color="#444444", edgecolor="gray", linewidth=0.3)
        elif ev == "OCIOSA":
            ax.barh(y, fim - ini, left=ini, color="white", hatch="//", edgecolor="gray", linewidth=0.3)
        else:
            ax.barh(y, fim - ini, left=ini, color=plt.cm.tab20(int(ev[1:]) % 20), edgecolor="gray", linewidth=0.3)
            if fim - ini >= 3:
                ax.text((ini + fim) / 2, y, ev[1:], ha="center", va="center", fontsize=6)
    ax.set_yticks(range(ncpus))
    ax.set_yticklabels([f"CPU {c}" for c in reversed(range(ncpus))])
    ax.set_xlim(0, 200)
    ax.set_xticks(range(0, 201, 10))
    ax.set_xlabel("tempo (cinza = TROCA, hachurado = OCIOSA, numero = pid)")
    ax.set_title(os.path.basename(arq)[6:-4])
    fig.tight_layout()
    fig.savefig("resultados/figuras/" + os.path.basename(arq)[:-4] + ".png", dpi=150)
    plt.close(fig)

print("pronto: resultados/tabela_resumo.csv e resultados/figuras/")