"""Audita o lote completo e gera uma tabela Markdown para consulta."""
import csv
import argparse
import hashlib
import json
import math
import re
from collections import Counter
from pathlib import Path
import statistics

from executar_benchmark import ROOT, validar


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'results/bb5_all_unproven')
    args = parser.parse_args()
    directory = args.output.resolve()
    with (ROOT/'instances/literature_pdsvrp_c.csv').open(encoding='utf-8') as stream:
        catalog = list(csv.DictReader(stream))
    with (directory/'comparison.csv').open(encoding='utf-8') as stream:
        rows = list(csv.DictReader(stream))
    assert len(rows) == 180
    expected = {(r['instance'], r['trucks']): r for r in catalog}
    assert len({(r['instance'], r['trucks']) for r in rows}) == 180
    gaps = []
    bb_limitados = 0
    for row in rows:
        key = row['instance'], row['trucks']
        assert all(row[k] == value for k, value in expected[key].items())
        if row['status'] == 'fora_do_lote':
            assert not row['found'] and not row['seed'] and row['proven'] == '0'
            continue
        if int(row['proven']):
            assert row['status'] == 'ignorada_otimo_comprovado'
            assert not row['found'] and not row['seed'] and float(row['seconds']) == 0
            continue
        assert row['found'] and not row['status'].startswith('erro')
        certificate = json.loads((directory/'solutions'/f'{key[0]}_s{key[1]}.json').read_text())
        value = validar(certificate, key[0], int(key[1]))
        assert math.isclose(value, float(row['found']), abs_tol=1e-6)
        gap = 100 * (value-float(row['ub'])) / float(row['ub'])
        assert math.isclose(gap, float(row['gap_percent']), abs_tol=0.000501)
        gaps.append(gap)
        log = (directory/'logs'/f'{key[0]}_s{key[1]}.log').read_text(errors='replace')
        if row['status'] == 'concluido':
            assert 'Makespan final:' in log
            stat = re.search(r'ESTATISTICAS .*bbLimitados=(\d+)', log)
            if stat: bb_limitados += int(stat[1])
    metadata = json.loads((directory/'run.json').read_text())
    assert metadata['executed'] == len(gaps) and metadata['skipped'] == 60
    counts = Counter(r['status'] for r in rows)
    summary = dict(configurations=180, executed=len(gaps), skipped=60,
                   not_selected=metadata.get('not_selected', 0),
                   feasible_certificates=len(gaps), matched_or_improved_bks=sum(g <= 1e-7 for g in gaps),
                   bb_calls_limited_by_nodes=bb_limitados,
                   mean_gap_percent=statistics.mean(gaps), median_gap_percent=statistics.median(gaps),
                   min_gap_percent=min(gaps), max_gap_percent=max(gaps), statuses=dict(counts))
    (directory/'summary.json').write_text(json.dumps(summary, indent=2)+'\n', encoding='utf-8')
    prazo = ('Sem limite de tempo; encerramento pelo resfriamento do SA.' if not metadata['seconds'] else
             f'Limite de {metadata["seconds"]:g} segundos de parede por configuracao.')
    lines = ['# Comparacao PDSVRP-C — lote de 30/10/2026', '',
             'Quantidade de drones sorteada pelo SA; BB otimiza somente a ordem e os horarios.', '',
             f'Foram executadas {len(gaps)} configuracoes abertas. {prazo} '
             f'Processos concorrentes: {metadata["workers"]}. '
             'As 60 configuracoes com otimo comprovado foram ignoradas. '
             f'Casos abertos fora do lote: {metadata.get("not_selected", 0)}.', '',
             f'Certificados viaveis validados: {len(gaps)}/{len(gaps)}. Casos que igualaram ou melhoraram '
             f'o BKS: {summary["matched_or_improved_bks"]}. Gap medio: {summary["mean_gap_percent"]:.3f}%; '
             f'mediano: {summary["median_gap_percent"]:.3f}%.', '',
             f'BB apos 5 tentativas sem melhora global, sobre a melhor solucao. '
             f'Orcamento de 100.000 nos por chamada; chamadas que atingiram o limite: {bb_limitados}. '
             'Nesses casos o cronograma e viavel, mas nao ha prova de otimo isolado dos drones.', '',
             'Em `limite_30s`, o valor e o melhor incumbente completo antes do corte externo, '
             'nao o resultado de um SA necessariamente concluido. `--` nos otimos significa '
             'que o SA nao foi executado, e nao que tenha encontrado o valor publicado.', '',
             'BKS: [MIC 2024, tabela 1](https://doi.org/10.1007/978-3-031-62912-9_13) para grandes; '
             '[COR 2024, tabelas 3 e 4](https://doi.org/10.1016/j.cor.2023.106514) para pequenas. '
             'Sao as referencias mais recentes localizadas. Ver ../../references/README.md para '
             'as tres melhorias da coluna VRP e a ressalva de correspondencia dos tempos de drones de 2025.', '',
             '| Instancia | Caminhoes | LB | BKS literatura | Encontrado | Gap (%) | Status |',
             '|---|---:|---:|---:|---:|---:|---|']
    for r in rows:
        lines.append(f'| {r["instance"]} | {r["trucks"]} | {r["lb"]} | {r["ub"]} | '
                     f'{r["found"] or "--"} | {r["gap_percent"] or "--"} | {r["status"]} |')
    (directory/'comparison.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    names = ['main.c', 'main.h', 'cv.c', 'cv.h', 'drone_bb.c', 'SA.c', 'SA.h',
             'lerInstancias.c', 'lerInstancias.h', 'pdsvrp_sa.exe',
             'scripts/executar_benchmark.py', 'scripts/preparar_literatura.py',
             'instances/literature_pdsvrp_c.csv']
    hashes = {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in names}
    (directory/'sha256.json').write_text(json.dumps(hashes, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
