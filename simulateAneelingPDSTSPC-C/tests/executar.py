"""Compila os testes C e verifica a integracao; requer apenas Python e GCC."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def run(*command):
    """Interrompe na primeira falha e conserva a saida de diagnostico."""
    subprocess.run(command, cwd=ROOT, check=True, timeout=120)


def main():
    """Isola objetos e executaveis de teste em uma pasta propria do workspace."""
    work = ROOT/'tmp'
    work.mkdir(exist_ok=True)
    flags = ['-O2', '-std=c11', '-Wall', '-Wextra', '-pedantic', '-I.']
    with tempfile.TemporaryDirectory(prefix='testes-', dir=work) as folder:
        base = Path(folder).resolve()
        assert base.is_relative_to(work.resolve())
        objects = []
        for source in ['SA.c', 'cv.c', 'lerInstancias.c']:
            obj = str(base/(Path(source).stem+'.o'))
            run('gcc', *flags, '-c', source, '-o', obj)
            objects.append(obj)
        checks = sorted((ROOT/'tests').glob('test_*.c'))
        for source in checks:
            exe = str(base/(source.stem+'.exe'))
            run('gcc', *flags, str(source), *objects, '-lm', '-o', exe)
            run(exe)
            print(source.stem+': OK', flush=True)
        run(sys.executable, str(ROOT/'tests/test_relatorio.py'))
    print(f'{len(checks)} testes C e integracao: aprovados.', flush=True)


if __name__ == '__main__':
    main()
