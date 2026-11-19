"""Compila todos os modulos C no ambiente atual, sem iniciar o algoritmo."""
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ('main.c', 'lerInstancias.c', 'cv.c', 'drone_bb.c', 'SA.c')


def main():
    """Gera ELF no Linux/WSL e PE no Windows; apresenta erros reais do GCC."""
    compiler = shutil.which('gcc')
    if not compiler:
        raise SystemExit('GCC nao encontrado no PATH do ambiente atual.')
    target = ROOT/('pdsvrp_sa.exe' if os.name == 'nt' else 'pdsvrp_sa')
    # Verifica o destino do compilador: no WSL, um GCC Windows nao gera ELF.
    machine = subprocess.run([compiler, '-dumpmachine'], check=True,
                             capture_output=True, text=True).stdout.strip().lower()
    windows = any(tag in machine for tag in ('mingw', 'windows', 'cygwin'))
    if (os.name == 'nt') != windows:
        raise SystemExit(f'GCC {machine} nao corresponde ao ambiente atual. '
                         'Use GCC Linux no WSL ou MinGW GCC no Windows.')
    command = [compiler, '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2',
               *SOURCES, '-o', str(target), '-lm']
    print('Compilando todos os modulos C...', flush=True)
    result = subprocess.run(command, cwd=ROOT)
    if result.returncode:
        raise SystemExit(result.returncode)
    print(f'Compilado: {target}')
    print('Para testar: .\\pdsvrp_sa.exe --list' if os.name == 'nt'
          else 'Para testar: ./pdsvrp_sa --list')


if __name__ == '__main__':
    main()
