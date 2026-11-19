"""Confere entradas contra o ZIP oficial e regenera o inventario de hashes."""
import csv
import hashlib
from io import BytesIO
from pathlib import Path
from zipfile import ZipFile

from executar_benchmark import dados_instancia, exigir

ROOT = Path(__file__).resolve().parents[1] / 'instances'


def main():
    """Compara bytes com o pacote oficial, sem precisar extrai-lo no projeto."""
    inventory = []
    with ZipFile(ROOT/'archives/pdstsp-c-pdsop-c-2025.zip') as outer:
        official_data = outer.read('PDSOP-c/data.zip')
    official = ZipFile(BytesIO(official_data))
    for path in sorted((ROOT / 'benchmark').glob('*/*.txt')):
        _, m, nodes, times = dados_instancia(path.stem)
        n = int(path.stem.split('-')[0])
        exigir(path.read_bytes() == official.read('data/instances/tsp/'+path.name),
               f'instancia diverge do ZIP oficial: {path.name}')
        tau = ROOT / 'drone_times' / ('tau_kj_' + path.name)
        exigir(tau.read_bytes() == official.read('data/instances/tau_jk/'+tau.name),
               f'tabela diverge do ZIP oficial: {tau.name}')
        inventory.append(dict(instance=path.stem, size=path.parent.name, customers=n,
                              drones=m, instance_file=path.relative_to(ROOT).as_posix(),
                              times_file=tau.relative_to(ROOT).as_posix(),
                              feasible_customer_k_pairs=sum(t > 0 for row in times for t in row),
                              instance_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                              times_sha256=hashlib.sha256(tau.read_bytes()).hexdigest()))
    official.close()
    exigir(len(inventory) == 60 and len(list((ROOT/'drone_times').glob('*.txt'))) == 60,
           'quantidade de arquivos oficiais')
    with (ROOT / 'inventory.csv').open('w', newline='', encoding='utf-8') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(inventory[0]))
        writer.writeheader()
        writer.writerows(inventory)
    print(f'Validated {len(inventory)} instances and 60 drone-time tables; '
          f'{sum(row["customers"] for row in inventory)} customers in total.')


if __name__ == '__main__':
    main()
