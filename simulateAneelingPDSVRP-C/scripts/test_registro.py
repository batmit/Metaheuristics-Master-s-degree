"""Regressao da persistencia e do recebimento de checkpoints ao vivo."""
import json
import contextlib
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
import types
import unittest
from unittest.mock import patch

import executar_benchmark as benchmark


class RegistroTests(unittest.TestCase):
    def test_replace_bloqueado_temporariamente_tenta_novamente(self):
        with tempfile.TemporaryDirectory(dir=benchmark.ROOT) as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            registro.salvar()
            real_replace = Path.replace
            calls = []
            def replace(source, target):
                calls.append(source)
                if len(calls) < 3:
                    raise PermissionError('bloqueio temporario')
                return real_replace(source, target)
            with patch.object(Path, 'replace', replace), patch.object(benchmark.time, 'sleep'):
                registro.salvar()
            self.assertEqual(len(calls), 3)
            self.assertEqual(json.loads(registro.path.read_text()), registro.data)
            self.assertEqual(list(Path(directory).glob('*.tmp')), [])

    def test_bloqueio_persistente_preserva_anterior_e_copia_recuperavel(self):
        with tempfile.TemporaryDirectory(dir=benchmark.ROOT) as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            registro.salvar()
            before = registro.path.read_bytes()
            with patch.object(benchmark, 'validar', return_value=100), \
                 patch.object(Path, 'replace', side_effect=PermissionError('bloqueado')) as replace, \
                 patch.object(benchmark.time, 'sleep'):
                with self.assertRaisesRegex(PermissionError, 'copia recuperavel'):
                    registro.atualizar('50-r-e', 4, {'makespan': 100}, 42, False)
            self.assertEqual(replace.call_count, 8)
            self.assertEqual(registro.path.read_bytes(), before)
            self.assertIsNone(registro.data['50-r-e_s4']['best'])
            copies = list(Path(directory).glob('best.*.tmp'))
            self.assertEqual(len(copies), 1)
            self.assertEqual(json.loads(copies[0].read_text())['50-r-e_s4']['best']['makespan'], 100)

    def test_falha_no_checkpoint_nao_repete_gravacao_no_finally(self):
        real_popen = subprocess.Popen
        def launch(*args, **kwargs):
            return real_popen([sys.executable, '-c',
                               'print(\'CHECKPOINT {"makespan": 100}\', flush=True)'], **kwargs)
        with tempfile.TemporaryDirectory(dir=benchmark.ROOT) as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            error = PermissionError('erro original')
            with patch.object(benchmark.subprocess, 'Popen', side_effect=launch), \
                 patch.object(benchmark, 'validar', return_value=100), \
                 patch.object(registro, 'salvar', side_effect=error) as save:
                with self.assertRaises(PermissionError) as raised:
                    benchmark.executar({'instance': '50-r-e', 'trucks': '4'},
                                       types.SimpleNamespace(seed=42, seconds=0), registro, 0)
            self.assertIs(raised.exception, error)
            self.assertEqual(save.call_count, 1)

    def test_tempos_de_cada_repeticao_persistem_sem_recorde(self):
        real_popen = subprocess.Popen
        def launch(*args, **kwargs):
            return real_popen([sys.executable, '-c', 'import time; time.sleep(0.1)'], **kwargs)
        with tempfile.TemporaryDirectory(dir=benchmark.ROOT) as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            with patch.object(benchmark.subprocess, 'Popen', side_effect=launch):
                for repetition in range(2):
                    benchmark.executar({'instance': '50-r-e', 'trucks': '4'},
                                       types.SimpleNamespace(seed=42, seconds=0), registro, repetition)
            loaded = benchmark.Registro(registro.path)
            entry = loaded.data['50-r-e_s4']
            self.assertIsNone(entry['best'])
            self.assertEqual([r['repetition'] for r in entry['runs']], [1, 2])
            for run in entry['runs']:
                self.assertGreaterEqual(run['seconds'], 0.1)
                self.assertEqual(run['status'], 'completed')

    def test_sigint_sem_traceback_preserva_registro_e_libera_bloqueio(self):
        """Um SIGINT real durante a entrada deve encerrar com codigo 130."""
        with tempfile.TemporaryDirectory(dir=benchmark.ROOT) as directory:
            path = Path(directory)/'best.json'
            path.write_text('{"preservar": true}\n', encoding='utf-8')
            before = path.read_bytes()
            code = ("import signal, sys; sys.path.insert(0, " + repr(str(benchmark.ROOT/'scripts')) + "); "
                    "import executar_benchmark as b; from pathlib import Path; "
                    "b.CENTRAL=Path(" + repr(str(path)) + "); "
                    "b.main=lambda: signal.raise_signal(signal.SIGINT); b.entrada()")
            result = subprocess.run([sys.executable, '-c', code], capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 130)
            self.assertEqual(result.stdout, '')
            self.assertIn('Execucao interrompida', result.stderr)
            self.assertNotIn('Traceback', result.stderr)
            self.assertEqual(path.read_bytes(), before)
            with benchmark.bloquear_execucao(path.parent/'.execution.lock'):
                pass

    def test_preserva_melhor_e_recarrega(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'best.json'
            with patch.object(benchmark, 'validar', side_effect=lambda c, *args: c['makespan']):
                registro = benchmark.Registro(path)
                self.assertTrue(registro.atualizar('50-r-e', 4, {'makespan': 120}, 1, False))
                self.assertFalse(registro.atualizar('50-r-e', 4, {'makespan': 130}, 2, False))
                self.assertFalse(registro.atualizar('50-r-e', 4, {'makespan': 120}, 3, False))
                self.assertTrue(registro.atualizar('50-r-e', 4, {'makespan': 108}, 4, False))
                loaded = benchmark.Registro(path)
                self.assertEqual(loaded.data['50-r-e_s4']['best'], {'makespan': 108, 'seed': 4})

    def test_invalido_nao_altera_arquivo(self):
        with tempfile.TemporaryDirectory() as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            registro.salvar()
            before = registro.path.read_bytes()
            with patch.object(benchmark, 'validar', side_effect=AssertionError):
                with self.assertRaises(AssertionError):
                    registro.atualizar('50-r-e', 4, {'makespan': 1}, 1, False)
            self.assertEqual(before, registro.path.read_bytes())

    def test_falha_de_gravacao_permite_tentar_a_mesma_melhora(self):
        with tempfile.TemporaryDirectory() as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            with patch.object(benchmark, 'validar', side_effect=lambda c, *args: c['makespan']):
                registro.atualizar('50-r-e', 4, {'makespan': 120}, 1, False)
                before = registro.path.read_bytes()
                with patch.object(registro, 'salvar', side_effect=OSError('disco indisponivel')):
                    with self.assertRaises(OSError):
                        registro.atualizar('50-r-e', 4, {'makespan': 108}, 2, False)
                self.assertEqual(before, registro.path.read_bytes())
                self.assertEqual(registro.data['50-r-e_s4']['best']['makespan'], 120)
                self.assertTrue(registro.atualizar('50-r-e', 4, {'makespan': 108}, 2, False))

    def test_certificado_salvo_independe_de_mutacoes_do_worker(self):
        with tempfile.TemporaryDirectory() as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            cert = {'makespan': 120, 'routes': [[1, 2]], 'missions': []}
            with patch.object(benchmark, 'validar', return_value=120):
                registro.atualizar('50-r-e', 4, cert, 1, False)
            cert['routes'][0].clear()
            self.assertEqual(registro.data['50-r-e_s4']['best']['routes'], [[1, 2]])

    def test_bloqueio_identifica_dono_e_libera_apos_encerramento(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'execution.lock'
            code = ("import sys; sys.path.insert(0, " + repr(str(benchmark.ROOT/'scripts')) + "); "
                    "from executar_benchmark import bloquear_execucao; "
                    "from pathlib import Path; "
                    "ctx=bloquear_execucao(Path(" + repr(str(path)) + ")); "
                    "ctx.__enter__(); print('READY', flush=True); sys.stdin.readline()")
            with subprocess.Popen([sys.executable, '-u', '-c', code], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, text=True) as owner:
                try:
                    self.assertEqual(owner.stdout.readline().strip(), 'READY')
                    with self.assertRaisesRegex(SystemExit, f'PID {owner.pid}'):
                        with benchmark.bloquear_execucao(path):
                            self.fail('Segundo executor conseguiu bloquear')
                finally:
                    owner.communicate('encerrar\n', timeout=5)
            # O arquivo existente nao representa um bloqueio abandonado.
            with benchmark.bloquear_execucao(path):
                pass

    def test_cancelamento_encerra_worker_sem_aguardar_fim_da_busca(self):
        real_popen = subprocess.Popen
        def launch(*args, **kwargs):
            return real_popen([sys.executable, '-c', 'import time; time.sleep(30)'], **kwargs)
        with tempfile.TemporaryDirectory() as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            cancel = threading.Event()
            timer = threading.Timer(0.3, cancel.set)
            timer.start()
            started = time.monotonic()
            try:
                with patch.object(benchmark.subprocess, 'Popen', side_effect=launch):
                    benchmark.executar({'instance': '50-r-e', 'trucks': '4'},
                                       types.SimpleNamespace(seed=42, seconds=0, cancel_event=cancel), registro, 0)
            finally:
                timer.cancel()
                timer.join()
            self.assertLess(time.monotonic()-started, 5)

    def test_erro_inclui_diagnostico_do_processo(self):
        real_popen = subprocess.Popen
        def launch(*args, **kwargs):
            return real_popen([sys.executable, '-c',
                               "import sys; print('Falha especifica do worker'); sys.exit(7)"], **kwargs)
        with tempfile.TemporaryDirectory() as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            with patch.object(benchmark.subprocess, 'Popen', side_effect=launch):
                with self.assertRaisesRegex(RuntimeError, 'Falha especifica do worker'):
                    benchmark.executar({'instance': '50-r-e', 'trucks': '4'},
                                       types.SimpleNamespace(seed=42, seconds=0), registro, 0)

    def test_checkpoint_persistido_antes_do_fim_do_processo(self):
        # Certificado viavel so com caminhoes, independente do historico salvo.
        _, _, nodes, _ = benchmark.dados_instancia('50-r-e')
        routes = [list(range(1, len(nodes))), [], [], []]
        path = [0, *routes[0], 0]
        speed = benchmark.dados_instancia('50-r-e')[0]
        value = sum((abs(nodes[a][1]-nodes[b][1]) + abs(nodes[a][2]-nodes[b][2]))
                    / speed * 60 for a, b in zip(path, path[1:]))
        cert = dict(routes=routes, missions=[], makespan=value)
        real_popen = subprocess.Popen
        process = None
        observed = []
        output = io.StringIO()
        def launch(*args, **kwargs):
            nonlocal process
            code = "import time; print(" + repr('CHECKPOINT '+json.dumps(cert)) + ",flush=True); time.sleep(0.5)"
            process = real_popen([sys.executable, '-u', '-c', code], **kwargs)
            return process
        with tempfile.TemporaryDirectory() as directory:
            registro = benchmark.Registro(Path(directory)/'best.json')
            update = registro.atualizar
            def receive(*args, **kwargs):
                changed = update(*args, **kwargs)
                observed.append((process.poll(), registro.path.exists(), '50-r-e (S=4)' in output.getvalue()))
                return changed
            registro.atualizar = receive
            with patch.object(benchmark.subprocess, 'Popen', side_effect=launch), contextlib.redirect_stdout(output):
                benchmark.executar({'instance':'50-r-e', 'trucks':'4'},
                                   types.SimpleNamespace(seed=42, seconds=0), registro, 0)
            self.assertEqual(observed, [(None, True, False)])
            self.assertEqual(output.getvalue(), '')


if __name__ == '__main__':
    unittest.main()
