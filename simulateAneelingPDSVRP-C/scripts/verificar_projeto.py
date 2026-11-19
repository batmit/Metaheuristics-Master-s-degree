"""Reproduz verificacoes sem executar o lote completo nem alterar os melhores."""
import contextlib
import io
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from executar_benchmark import ROOT, Registro, catalogo, main


def executar(command, cwd=ROOT):
    """Falha imediatamente e conserva o diagnostico do comando que falhou."""
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f'{command}\n{result.stdout}\n{result.stderr}')
    return result.stdout.strip()


def main_verificacao():
    """Compila com avisos como erros; verifica entradas, vizinhos e certificados."""
    output = ROOT/'output'
    output.mkdir(exist_ok=True)
    # O temporario fica dentro do projeto para funcionar tambem em sandboxes.
    with tempfile.TemporaryDirectory(dir=output) as temporary:
        directory = Path(temporary)
        suffix = '.exe' if os.name == 'nt' else ''
        common = ['gcc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2',
                  'lerInstancias.c', 'cv.c', 'drone_bb.c', 'SA.c']
        binaries = {}
        for name in ('verificar_bb', 'verificar_bb5', 'verificar_leitura',
                     'verificar_sa', 'verificar_vizinhos'):
            binary = directory/(name+suffix)
            executar([*common, f'scripts/{name}.c', '-o', str(binary), '-lm'])
            binaries[name] = str(binary)
        binary = directory/('pdsvrp_sa'+suffix)
        executar([*common, 'main.c', '-o', str(binary), '-lm'])
        print('Compilacao estrita: OK', flush=True)
        executar([sys.executable, '-m', 'unittest', 'discover', '-s', 'scripts', '-p', 'test_*.py'])
        executar([sys.executable, 'scripts/validate_instances.py'])
        for name in ('verificar_bb', 'verificar_bb5'):
            print(executar([binaries[name]]), flush=True)
        configurations = 0
        for path in sorted((ROOT/'instances/benchmark').glob('*/*.txt')):
            tau = ROOT/f'instances/drone_times/tau_kj_{path.name}'
            args = [str(path), str(tau)]
            executar([binaries['verificar_leitura'], *args])
            configurations += int(executar([binaries['verificar_vizinhos'], *args]))
        print(f'Vizinhos: {configurations*500} em {configurations} configuracoes; OK', flush=True)
        for name, size in (('15-rc-c', 'small'), ('50-r-e', 'large'), ('200-r-e', 'large')):
            executar([binaries['verificar_sa'], str(ROOT/f'instances/benchmark/{size}/{name}.txt'),
                      str(ROOT/f'instances/drone_times/tau_kj_{name}.txt')])
        # Teste integrado limitado: usa registro isolado e o binario recem-compilado.
        # Nao altera results/best.json e nao inicia o lote de 120 configuracoes.
        isolated = Registro(directory/'best.json')
        table = io.StringIO()
        with patch('executar_benchmark.Registro', return_value=isolated), \
             patch('executar_benchmark.localizar_executavel', return_value=binary), \
             patch.object(sys, 'argv', ['benchmark', '--limit', '1', '--seconds', '1', '--workers', '1']), \
             contextlib.redirect_stdout(table):
            main()
        lines = table.getvalue().splitlines()
        first = next(row for row in catalogo() if not int(row['proven']))
        expected = f"{first['instance']} (S={first['trucks']})"
        if (len(lines) != 2 or not lines[0].startswith('Instancia') or
                any(not line.startswith(expected) for line in lines[1:])):
            raise RuntimeError('Saida integrada deve mostrar apenas a configuracao executada, sem tabela inicial salva.')
        new_certificates = sum(e['best'] is not None for e in isolated.data.values())
        if new_certificates != 1:
            raise RuntimeError('Execucao integrada nao produziu certificado validado.')
        # O lancador usa um catalogo isolado: a auditoria nao disputa o bloqueio
        # do usuario nem depende de ele estar ou nao executando uma otimizacao.
        launcher_root = directory/'launcher'
        (launcher_root/'scripts').mkdir(parents=True)
        (launcher_root/'instances').mkdir()
        shutil.copy2(ROOT/'scripts/executar_benchmark.py', launcher_root/'scripts/executar_benchmark.py')
        shutil.copy2(ROOT/'instances/literature_pdsvrp_c.csv', launcher_root/'instances/literature_pdsvrp_c.csv')
        # O catalogo agora confere um limite fisico local antes de exibir gaps.
        shutil.copytree(ROOT/'instances/benchmark', launcher_root/'instances/benchmark')
        shutil.copytree(ROOT/'instances/drone_times', launcher_root/'instances/drone_times')
        launcher = executar([str(binary), '--list'], cwd=launcher_root).splitlines()
        if len(launcher) != len(catalogo())+1 or not launcher[0].startswith('Instancia'):
            raise RuntimeError('Lancador C nao apresenta somente a tabela.')
    registro = Registro()
    summary = dict(instancias=60, configuracoes=configurations, vizinhos_validados=configurations*500,
                   testes_python=unittest.defaultTestLoader.discover(str(ROOT/'scripts'), pattern='test_*.py').countTestCases(),
                   certificados_salvos_validados=sum(e['best'] is not None for e in registro.data.values()),
                   certificados_integracao=new_certificates, compilacao='C11, Wall, Wextra, Wpedantic, Werror, O2',
                   trajetorias_sa=['15-rc-c', '50-r-e', '200-r-e'],
                   saida_apenas_tabela=True, validacao_python_otimizado=True,
                   limite='Viabilidade relativa aos dados locais; sem prova de otimo global nem recalculo fisico dos voos.')
    (output/'verificacao.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(json.dumps(summary, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main_verificacao()
