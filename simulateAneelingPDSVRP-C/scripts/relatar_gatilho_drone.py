"""Report the isolated BB-trigger experiment and preserve the production version."""
import csv
import hashlib
import json
import statistics
from executar_benchmark import ROOT


def main():
    out=ROOT/'output/experimento_gatilho_drone'
    summary=json.loads((out/'summary.json').read_text())
    protocol=json.loads((out/'protocol.json').read_text())
    pairs=list(csv.DictReader((out/'paired.csv').open()))
    rows=list(csv.DictReader((out/'comparison.csv').open()))
    runs=[json.loads(line) for line in (out/'runs.jsonl').read_text().splitlines()]
    completed=[r for r in pairs if r['both_completed']=='True']
    stats={}
    for version in ('original','drone'):
        group=[r for r in runs if r['version']==version]
        stats[version]=dict(mean_global=statistics.mean(r['best']['makespan'] for r in group),
            mean_drone=statistics.mean(r['drone_time'] for r in group),
            mean_wall=statistics.mean(r['wall_seconds'] for r in group))
    complete_stats={}
    for version,field in [('original','original_seconds'),('drone','new_seconds')]:
        complete_stats[version]=statistics.mean(float(r[field]) for r in completed) if completed else None
    original_bytes=(ROOT/'SA.c').read_bytes()==(out/'SA_original.c').read_bytes()
    extra=dict(means=stats,paired_completed=len(completed),paired_completed_mean_wall=complete_stats,
        original_source_preserved=original_bytes,
        source_sha256=hashlib.sha256((ROOT/'SA.c').read_bytes()).hexdigest())
    (out/'additional_metrics.json').write_text(json.dumps(extra,indent=2)+'\n')
    decision='Candidata atende ao criterio; revisar antes de promover.' if summary['keep_candidate'] else 'Nao promover a candidata; manter o gatilho anterior.'
    report=f'''# Experimento: BB apos estagnacao dos drones — 17/11/2026

## Decisao

{decision}
SA.c identico byte a byte ao anterior: {original_bytes}.
Nenhuma versao experimental foi instalada no executavel principal.

## Alteracao testada

Original: apos cinco tentativas sem melhora do makespan global, tenta refinar
a melhor solucao com BB, preservando as verificacoes de missoes existentes e
branchBoundAplicado.

Candidata: conta apenas tentativas que alteram a quantidade de drones de uma
missao ou transferem cliente entre caminhao e drones. Compara o tempo dos drones
do candidato ao da solucao corrente, antes da aceitacao. Uma reducao zera o
contador; uma tentativa sem reducao o incrementa; movimentos somente dos
caminhoes nao contam. Ao atingir cinco, aplica o mesmo refinamento BB na melhor
solucao e zera a contagem. Conta tambem candidatos rejeitados pelo SA. Nao usa
um recorde historico de tempo dos drones, pois o estado sem missoes tem tempo
zero. Isso mede estagnacao local observada, nao prova de otimalidade.

O escalonamento guloso continua necessario para avaliar cada candidato; somente
o gatilho do BB foi modificado. Permaneceram identicos: dados, matriz Manhattan,
tempos oficiais, movimentos, calibracao, resfriamento, meta dos caminhoes,
limite de 100.000 nos e alvo do BB (melhor solucao).

## Protocolo

180 configuracoes, tres sementes pareadas por configuracao, duas versoes:
1.080 execucoes. Limite de {protocol['seconds']} segundos por execucao, seis processos
concorrentes. As duas versoes de cada par rodam sequencialmente, alternando
a ordem entre configuracoes/repeticoes. Semente = 42 + 10*n + S + 1000003*rep,
rep = 0,1,2. Nenhuma solucao anterior e fornecida. Resultados nao atualizam
results/best.json. Fontes isolados e executaveis preservados no diretorio do teste.

Criterio registrado antes dos resultados: adotar somente se a variacao media
percentual do makespan for negativa, o tempo medio dos drones nao aumentar e o
teste de sinais unilateral sobre medias por configuracao resultar p < 0,05.
Configuracoes da mesma instancia compartilham dados; o teste e uma triagem
exploratoria, nao prova estatistica de superioridade independente.

## Resultados

| Indicador | Original | Candidata |
| --- | ---: | ---: |
| Makespan medio, minutos | {stats['original']['mean_global']:.6f} | {stats['drone']['mean_global']:.6f} |
| Tempo medio dos drones, minutos | {stats['original']['mean_drone']:.6f} | {stats['drone']['mean_drone']:.6f} |
| Execucoes completas | {summary['completed']['original']} | {summary['completed']['drone']} |

- Medias por configuracao: {summary['wins']} melhoras, {summary['losses']} pioras,
  {summary['ties']} empates no makespan.
- Por semente pareada: {summary['paired_wins']} melhoras, {summary['paired_losses']} pioras,
  {540-summary['paired_wins']-summary['paired_losses']} empates.
- Variacao media percentual do makespan (candidata-original): {summary['mean_global_change_pct']:.6f}%.
- Variacao media do tempo dos drones: {summary['mean_drone_change_minutes']:.6f} minutos.
- Teste de sinais unilateral, p: {summary['sign_test_one_sided']:.6g}.
- Pares com ambas as execucoes completas: {len(completed)}.
- Tempo de parede da campanha: {summary['wall_seconds']:.1f} segundos.
- Certificados/checkpoints validados independentemente: {summary['checkpoints']}.

## Validacao e limites

Compilacao C11 estrita com avisos como erros nas duas versoes. Testes do contador
cobrem cinco tentativas sem melhora, reinicio por melhora, ignorar movimentos
de caminhao e transferencias caminhao-drone. Todos os checkpoints emitidos sao
validados pelo avaliador Python quanto a cobertura, tempos e capacidade.

O tempo dos drones isolado nao mede qualidade global: pode cair porque clientes
voltaram aos caminhoes. O makespan e a medida primaria. O limite curto, carga
do sistema e encerramentos por tempo introduzem variacao; os resultados nao
demonstram o comportamento com orcamentos longos. Tempos computacionais de
execucoes interrompidas nao representam tempo para completar o SA. Contadores
BB so existem nas execucoes completas; comparar somas brutas enviesaria a analise.

## Artefatos

output/experimento_gatilho_drone: protocol.json, summary.json,
additional_metrics.json, paired.csv (540 pares), comparison.csv (180 configuracoes),
runs.jsonl (certificados), SA_original.c e SA_drone.c.

Reproducao: python scripts/experimentar_gatilho_drone.py
Relatorio: python scripts/relatar_gatilho_drone.py

## Diferencas por configuracao

| Instancia | S | Media original | Media candidata | Variacao (%) |
| --- | ---: | ---: | ---: | ---: |
'''
    for r in rows:
        if abs(float(r['change_pct']))>1e-9:
            report+=f"| {r['instance']} | {r['trucks']} | {float(r['original_mean']):.3f} | {float(r['new_mean']):.3f} | {float(r['change_pct']):.3f} |\n"
    (ROOT/'docs/auditorias/gatilho_drone_20261117.md').write_text(report,encoding='utf-8')
    print(json.dumps(extra,indent=2))

if __name__=='__main__': main()
