"""Extrai tempos do preprint preservado, sem atribui-los a outra publicacao."""
import csv
import html
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def extrair():
    text = (ROOT/'references/cor2024.html').read_text(encoding='utf-8')
    results = {}
    for table, trucks in ((3, 2), (4, 3)):
        figure = re.search(fr'<figure id="S4.T{table}".*?</figure>', text, re.S)[0]
        for row in re.findall(r'<tr\b.*?</tr>', figure, re.S):
            cells = [html.unescape(re.sub(r'<[^>]+>', '', cell)).strip()
                     for cell in re.findall(r'<td\b[^>]*>(.*?)</td>', row, re.S)]
            if len(cells) != 11 or not re.fullmatch(r'\d+-[a-z]+-[a-z]+', cells[0]):
                continue
            candidates = []
            for i, model in zip((1, 4, 7), ('MILP+(16)', 'CP2+(16)', 'CP3+(16)')):
                bounds = re.findall(r'\d+(?:\.\d+)?', cells[i])
                if len(bounds) == 2 and re.fullmatch(r'\d+(?:\.\d+)?', cells[i+1]):
                    candidates.append((float(bounds[1]), float(cells[i+1]), model))
            results[f'{cells[0]}_s{trucks}'] = candidates
    return results


def main():
    candidates = extrair()
    with (ROOT/'instances/literature_pdsvrp_c.csv').open(encoding='utf-8') as stream:
        rows = list(csv.DictReader(stream))
    data = {}
    for row in rows:
        key = f"{row['instance']}_s{row['trucks']}"
        matches = [x for x in candidates.get(key, []) if x[0] == float(row['ub'])]
        if matches:
            _, seconds, model = min(matches, key=lambda x: x[1])
            data[key] = dict(seconds=seconds, time_kind='total',
                             time_source=f'COR-preprint-T{int(row["trucks"])+1}:{model}:Sec_tot')
        else:
            data[key] = dict(seconds=None, time_kind='unavailable',
                             time_source='Tempo da fonte do BKS nao confirmado')
    (ROOT/'references/literature_times.json').write_text(
        json.dumps(data, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
