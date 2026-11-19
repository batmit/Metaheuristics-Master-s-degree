"""Experimento pareado isolado: recompila copias e valida todos os checkpoints."""
import argparse
import concurrent.futures
import csv
import json
import os
from pathlib import Path
import statistics
import subprocess
import threading
import time

from executar_benchmark import ROOT, catalogo, validar, dados_instancia


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=2)
    parser.add_argument('--repetitions', type=int, default=3)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--output', type=Path, default=ROOT/'output/experimento_metricas')
    args = parser.parse_args()
    if args.seconds <= 0 or args.repetitions < 1 or args.workers < 1:
        parser.error('Limites devem ser positivos')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = (ROOT/'cv.c').read_text(encoding='utf-8')
    expression = '(dx + dy) / inst->truckSpeed * 60.0'
    if source.count(expression) != 1:
        raise RuntimeError('Expressao de distancia inesperada')
    binaries = {}
    for metric in ('manhattan', 'euclidean'):
        code = source.replace(expression, 'hypot(dx, dy) / inst->truckSpeed * 60.0') if metric == 'euclidean' else source
        copy = out/f'cv_{metric}.c'
        copy.write_text(code, encoding='utf-8')
        binary = out/(metric + ('.exe' if os.name == 'nt' else ''))
        subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2',
                        '-I', str(ROOT), 'main.c', 'lerInstancias.c', str(copy),
                        'drone_bb.c', 'SA.c', '-o', str(binary), '-lm'], cwd=ROOT, check=True)
        binaries[metric] = binary
    rows = [r for r in catalogo() if not int(r['proven'])]
    # Pre-carrega dados: o validador independente nao depende da matriz do C.
    for row in rows:
        dados_instancia(row['instance'])
    tasks = [(r, repetition, metric) for r in rows for repetition in range(args.repetitions)
             for metric in binaries]

    def run(task):
        row, repetition, metric = task
        name, trucks = row['instance'], int(row['trucks'])
        seed = (42 + int(name.split('-')[0])*10 + trucks + repetition*1000003) % 2**32
        command = [str(binaries[metric]), f'instances/benchmark/large/{name}.txt',
                   f'instances/drone_times/tau_kj_{name}.txt', str(trucks), str(seed)]
        started = time.perf_counter()
        best, count = None, 0
        timed_out = threading.Event()
        with subprocess.Popen(command, cwd=ROOT, env=dict(os.environ, PDSVRP_WORKER='1'),
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True) as process:
            def stop():
                if process.poll() is None:
                    timed_out.set()
                    process.kill()
            timer = threading.Timer(args.seconds, stop)
            timer.start()
            try:
                for line in process.stdout:
                    if line.startswith('CHECKPOINT '):
                        cert = json.loads(line[len('CHECKPOINT '):])
                        validar(cert, name, trucks, metric)
                        count += 1
                        if best is None or cert['makespan'] < best['makespan']:
                            best = cert
                process.wait()
                error = process.stderr.read()
            finally:
                timer.cancel()
                timer.join()
                if process.poll() is None:
                    process.kill()
                process.wait()
        if best is None or (process.returncode and not timed_out.is_set()):
            raise RuntimeError(f'{name}/{trucks}/{metric}: {error}; checkpoints={count}')
        return dict(instance=name, trucks=trucks, metric=metric, seed=seed,
                    repetition=repetition+1, seconds=time.perf_counter()-started,
                    status='timeout' if timed_out.is_set() else 'completed',
                    checkpoints=count, best=best)

    runs = []
    with (out/'runs.jsonl').open('w', encoding='utf-8') as log:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
            for result in pool.map(run, tasks):
                runs.append(result)
                log.write(json.dumps(result, allow_nan=False)+'\n')
                log.flush()
                if len(runs) % 40 == 0:
                    print(f'{len(runs)}/{len(tasks)} execucoes validadas', flush=True)
    comparison = []
    for row in rows:
        name, trucks, ub = row['instance'], int(row['trucks']), float(row['ub'])
        group = [r for r in runs if r['instance'] == name and r['trucks'] == trucks]
        m, e = (min(r['best']['makespan'] for r in group if r['metric'] == metric)
                for metric in binaries)
        comparison.append(dict(instance=name, trucks=trucks, literature_lb=float(row['lb']),
                               literature_ub=ub, manhattan=m, euclidean=e,
                               reduction_pct=100*(m-e)/m, gap_manhattan_pct=100*(m-ub)/ub,
                               difference_euclidean_literature_pct=100*(e-ub)/ub))
    with (out/'comparison.csv').open('w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(comparison[0]))
        writer.writeheader()
        writer.writerows(comparison)
    summary = dict(configurations=len(rows), runs=len(runs), repetitions=args.repetitions,
                   seconds_per_run=args.seconds, workers=args.workers, base_seed=42,
                   mean_reduction_pct=statistics.mean(r['reduction_pct'] for r in comparison),
                   mean_gap_manhattan_pct=statistics.mean(r['gap_manhattan_pct'] for r in comparison),
                   mean_difference_euclidean_literature_pct=statistics.mean(r['difference_euclidean_literature_pct'] for r in comparison),
                   euclidean_better=sum(r['euclidean'] < r['manhattan']-1e-7 for r in comparison),
                   euclidean_equal=sum(abs(r['euclidean']-r['manhattan']) <= 1e-7 for r in comparison),
                   manhattan_below_ub=sum(r['manhattan'] < r['literature_ub']-1e-7 for r in comparison),
                   euclidean_below_ub=sum(r['euclidean'] < r['literature_ub']-1e-7 for r in comparison),
                   euclidean_below_lb=sum(r['euclidean'] < r['literature_lb']-1e-7 for r in comparison),
                   checkpoints=sum(r['checkpoints'] for r in runs),
                   completed=sum(r['status'] == 'completed' for r in runs))
    (out/'summary.json').write_text(json.dumps(summary, indent=2)+'\n', encoding='utf-8')
    report = [
        '# Experimento: Manhattan versus Euclidiana', '',
        'Data: 17/11/2026. Unidade do objetivo: minutos. Menor makespan e melhor.', '',
        f'Foram executadas {len(rows)} configuracoes grandes (30 instancias, S=2,3,4,5), '
        f'{args.repetitions} sementes por metrica, {args.seconds:g} segundos por execucao, '
        f'{args.workers} processos simultaneos. Semente base 42; semente efetiva: '
        '(42 + 10*n + S + 1000003*repeticao) modulo 2^32, repeticao iniciando em zero.', '',
        'As duas versoes foram recompiladas com GCC -O2 a partir dos mesmos fontes. '
        'A unica mudanca na copia experimental de cv.c e substituir dx+dy por hypot(dx,dy). '
        'Os tempos oficiais de drones foram mantidos. Os certificados de cada checkpoint '
        'foram recalculados independentemente em Python, incluindo cobertura e capacidade. '
        'O executavel principal e results/best.json nao foram substituidos.', '',
        'Cada linha usa o melhor valor das repeticoes. As medias percentuais sao '
        'aritmeticas por configuracao; nao sao a razao das medias dos makespans.', '',
        '| Indicador | Resultado |', '| --- | ---: |',
        f"| Execucoes / checkpoints validados | {len(runs)} / {summary['checkpoints']} |",
        f"| Execucoes concluidas antes do limite | {summary['completed']} |",
        f"| Reducao media Euclidiana versus Manhattan | {summary['mean_reduction_pct']:.3f}% |",
        f"| Gap medio Manhattan versus UB publicado | {summary['mean_gap_manhattan_pct']:.3f}% |",
        f"| Diferenca media Euclidiana versus UB publicado | {summary['mean_difference_euclidean_literature_pct']:.3f}% |",
        f"| Euclidiana menor / igual a Manhattan | {summary['euclidean_better']} / {summary['euclidean_equal']} |",
        f"| Manhattan abaixo do UB publicado | {summary['manhattan_below_ub']} |",
        f"| Euclidiana abaixo do UB / LB publicado | {summary['euclidean_below_ub']} / {summary['euclidean_below_lb']} |", '',
        '## Interpretacao e limites', '',
        'Os agregados contra a literatura incluem todas as 120 configuracoes apenas '
        'como diferencas numericas. Os UBs de 126-rc-e/S=3,4,5 ja foram identificados '
        'como incompativeis com os dados Manhattan locais (ver README). Nao interpretar '
        'a media integral como gap oficial. A heuristica com corte curto pode obter '
        'resultado pior em Euclidiana, embora o custo de uma mesma rota nunca aumente.', '',
        'COR 2024 explicita Manhattan para caminhoes a 30 km/h e Euclidiana para drones. '
        'Portanto, o resultado euclidiano resolve uma variante com deslocamentos de caminhao '
        'mais curtos: diferencas negativas contra os UBs publicados nao sao recordes do '
        'problema original. Os LBs publicados tambem nao sao limites validos dessa variante. '
        'As referencias das grandes sao os valores MIC 2024 transcritos no catalogo local; '
        'nao foi feita uma busca exaustiva de novos BKS.', '',
        'O limite curto pode encerrar o SA durante a calibracao. O melhor checkpoint valido '
        'e conservado. A variacao do escalonamento do sistema e os quatro processos '
        'simultaneos afetam o numero de iteracoes; sementes iguais nao garantem trajetorias '
        'iguais entre metricas nem reproducao exata com corte por tempo. Este teste e '
        'exploratorio, sem prova de otimalidade ou significancia estatistica. A identidade '
        'dos tempos locais de drones com os usados nos artigos continua nao comprovada.', '',
        '## Resultados por configuracao', '',
        '| Instancia | S | UB literatura | Manhattan | Euclidiana | Reducao % | Dif. Eucl./lit. % |',
        '| --- | ---: | ---: | ---: | ---: | ---: | ---: |']
    for r in comparison:
        report.append(f"| {r['instance']} | {r['trucks']} | {r['literature_ub']:.3f} | "
                      f"{r['manhattan']:.3f} | {r['euclidean']:.3f} | {r['reduction_pct']:.3f} | "
                      f"{r['difference_euclidean_literature_pct']:.3f} |")
    report += ['', '## Fontes', '',
               '- COR 2024: https://doi.org/10.1016/j.cor.2023.106514',
               '- MIC 2024: https://doi.org/10.1007/978-3-031-62912-9_13',
               '- Catalogo local: instances/literature_pdsvrp_c.csv', '',
               'Reproducao: `python scripts/experimentar_metricas.py --seconds 2 --repetitions 3 --workers 4`', '',
               'Artefatos: comparison.csv (120 linhas), runs.jsonl (certificados por execucao), '
               'summary.json (agregados), cv_*.c e executaveis isolados.']
    (out/'relatorio.md').write_text('\n'.join(report)+'\n', encoding='utf-8')
    archive = ROOT/'docs/auditorias/experimento_metricas_20261117.md'
    archive.write_text('\n'.join(report)+'\n', encoding='utf-8')
    print(json.dumps(summary, indent=2), flush=True)


if __name__ == '__main__':
    main()
