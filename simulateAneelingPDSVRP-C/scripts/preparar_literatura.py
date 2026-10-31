"""Transcricao auditavel das tabelas COR 2024 (3/4) e MIC 2024 (1, p.140)."""
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# Nome, otimo 2 caminhoes, otimo 3 caminhoes. COR, versao publicada.
SMALL = '''15-r-e 92 92
15-rc-c 33 32
16-c-c 40 36
16-r-e 108 108
18-c-c 44 44
18-r-e 92 92
18-rc-c 46 40
19-c-c 36 36
20-c-c 40 40
20-r-c 48 37
20-r-e 72 72
20-rc-c 64 48
20-rc-e 80 68
21-c-c 40 36
21-r-e 76 76
23-c-e 80 80
23-r-c 60 48
24-c-e 60 60
24-r-e 100 100
24-rc-c 52 44
25-c-c 40 37
25-r-e 88 85
25-rc-e 76 66
26-r-c 70 56
27-c-c 52 36
27-c-e 68 68
27-rc-c 72 60
27-rc-e 76 76
29-rc-e 100 100
30-c-c 64 38'''
# Nome, quatro pares LB/UB das colunas New bounds, s=2,3,4,5.
LARGE = '''50-r-e 69 112 62 104 62 100 61 100
53-r-e 82 112 64 96 64 96 64 92
66-rc-e 73 112 57 100 44 100 46 100
67-c-c 38 48 27 52 21 48 21 52
68-rc-c 52 56 42 56 36 52 35 44
76-c-c 26 36 18 24 16 24 14 24
82-c-e 32 64 26 64 26 64 26 64
82-rc-c 62 100 48 80 38 60 33 60
88-c-e 58 84 39 76 39 72 38 72
91-r-c 75 124 56 100 45 76 38 68
99-rc-c 63 96 47 64 37 64 32 60
101-r-c 71 152 52 96 42 76 36 68
103-rc-c 69 124 49 88 39 76 37 68
105-rc-e 65 128 49 104 39 104 33 104
108-rc-e 79 152 58 124 46 120 39 120
114-rc-c 58 104 44 72 35 72 30 64
121-rc-e 70 132 52 96 42 92 34 92
126-rc-e 87 152 63 100 50 84 41 76
126-r-c 78 136 56 116 45 112 37 112
144-rc-c 67 116 50 116 40 72 34 92
154-c-c 35 40 24 36 18 36 15 36
165-r-c 88 184 68 132 54 108 47 84
167-r-e 100 180 73 132 58 120 49 116
173-r-c 85 176 65 128 54 96 43 92
173-rc-c 79 144 58 116 46 84 39 80
181-r-e 112 180 82 152 65 124 54 120
185-c-c 48 60 32 48 24 44 24 44
187-rc-e 100 160 74 124 58 148 48 128
198-c-c 32 36 22 36 16 36 16 36
200-r-e 105 216 77 148 60 120 52 120'''


def main():
    rows = []
    for line in SMALL.splitlines():
        name, *values = line.split()
        for trucks, value in zip((2, 3), map(int, values)):
            rows.append(dict(instance=name, trucks=trucks, lb=value, ub=value,
                             proven=1, source='COR2024-T' + str(trucks + 1)))
    for line in LARGE.splitlines():
        name, *values = line.split()
        values = list(map(int, values))
        for i, trucks in enumerate(range(2, 6)):
            lb, ub = values[2*i:2*i+2]
            source = 'MIC2024-T1-NewBounds'
            # A coluna VRP e uma solucao viavel, e nestes tres casos melhora
            # a coluna New bounds da mesma tabela. Preserva ambas em fontes.
            vrp = {('126-r-c', 3): 108, ('126-r-c', 4): 88, ('126-r-c', 5): 80}
            if (name, trucks) in vrp:
                ub = min(ub, vrp[name, trucks])
                source = 'MIC2024-T1-VRP'
            assert lb <= ub
            rows.append(dict(instance=name, trucks=trucks, lb=lb, ub=ub,
                             proven=int(lb == ub), source=source))
    expected = {(p.stem, s) for p in (ROOT/'instances/benchmark').glob('*/*.txt')
                for s in ((2, 3) if p.parent.name == 'small' else (2, 3, 4, 5))}
    assert len(rows) == 180 and {(r['instance'], r['trucks']) for r in rows} == expected
    assert sum(r['proven'] for r in rows) == 60
    path = ROOT/'instances/literature_pdsvrp_c.csv'
    with path.open('w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print('Catalogo: 180 configuracoes, 60 otimos comprovados, 120 abertas.')


if __name__ == '__main__':
    main()
