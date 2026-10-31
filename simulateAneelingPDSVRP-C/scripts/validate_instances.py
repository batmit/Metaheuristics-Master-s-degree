"""Validate official benchmark files and generate a provenance inventory."""
import csv
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'instances'


def main():
    inventory = []
    for path in sorted((ROOT / 'benchmark').glob('*/*.txt')):
        lines = path.read_text().splitlines()
        nodes = [[float(v) for v in line.split()] for line in lines[7:] if line.strip()]
        n = int(path.stem.split('-')[0])
        assert len(nodes) == n + 1, path
        assert [int(row[0]) for row in nodes] == list(range(n + 1)), path
        newer = ROOT / 'additional_2025/data/data/instances/tsp' / path.name
        assert path.read_bytes() == newer.read_bytes(), f'Changed instance: {path}'
        tau = ROOT / 'drone_times' / ('tau_kj_' + path.name)
        rows = [[int(v) for v in line.split()] for line in tau.read_text().splitlines()[1:] if line.strip()]
        m = int(lines[0].split(',')[1])
        assert len(rows) == n, tau
        assert [row[0] for row in rows] == list(range(1, n + 1)), tau
        assert all(len(row) == m + 1 for row in rows), tau
        assert all(t == -1 or t > 0 for row in rows for t in row[1:]), tau
        inventory.append(dict(instance=path.stem, size=path.parent.name, customers=n,
                              drones=m, instance_file=path.relative_to(ROOT).as_posix(),
                              times_file=tau.relative_to(ROOT).as_posix(),
                              feasible_customer_k_pairs=sum(t > 0 for row in rows for t in row[1:]),
                              instance_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                              times_sha256=hashlib.sha256(tau.read_bytes()).hexdigest()))
    assert len(inventory) == 60
    assert len(list((ROOT / 'drone_times').glob('*.txt'))) == 60
    with (ROOT / 'inventory.csv').open('w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(inventory[0]))
        writer.writeheader()
        writer.writerows(inventory)
    print(f'Validated {len(inventory)} instances and 60 drone-time tables; '
          f'{sum(row["customers"] for row in inventory)} customers in total.')


if __name__ == '__main__':
    main()
