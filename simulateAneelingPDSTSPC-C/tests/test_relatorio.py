"""Testa o lote completo sem argumentos usando prazo curto na compilacao de teste."""
import csv
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
HEADER = ['instancia', 'melhor_algoritmo', 'literatura_2024', 'literatura_2025', 'gap_percentual', 'tempo_ultima_execucao_s', 'tempo_literatura_2025_s']


class RelatorioTest(unittest.TestCase):
    def test_lote_completo_sem_argumentos(self):
        """Verifica 60 casos, saida progressiva, caminhos e melhores persistidos."""
        work = ROOT / 'tmp'
        work.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=work) as tmp:
            base = Path(tmp).resolve()
            # A limpeza automatica fica limitada a uma pasta do workspace.
            self.assertTrue(base.is_relative_to(work.resolve()))
            shutil.copytree(ROOT / 'instances', base / 'instances')
            exe = base / 'benchmark.exe'
            build = subprocess.run(['gcc', '-O2', '-std=c11', '-Wall', '-Wextra', '-pedantic',
                                    '-DSA_SEGUNDOS_POR_INSTANCIA=0.05', 'main.c', 'cv.c',
                                    'lerInstancias.c', 'SA.c', '-lm', '-o', str(exe)],
                                   cwd=ROOT, capture_output=True, text=True, timeout=60)
            self.assertEqual(build.returncode, 0, build.stderr)
            self.assertEqual(build.stderr, '')
            with (ROOT / 'instances/pdstsp-c/referencias.csv').open(encoding='utf-8-sig') as f:
                references = {r['instance']: r for r in csv.DictReader(f)}
            # Tabela reiniciada: referencias presentes, resultados ainda vazios.
            report = base / 'resultados/relatorio.csv'
            report.parent.mkdir()
            with report.open('w', newline='') as stream:
                writer = csv.DictWriter(stream, HEADER)
                writer.writeheader()
                writer.writerows({'instancia': name, 'melhor_algoritmo': '',
                                  'literatura_2024': ref['ub_2024'],
                                  'literatura_2025': ref['ub_2025'], 'gap_percentual': ''}
                                 for name, ref in references.items())
            # Diretorio de trabalho diferente do executavel: simula atalho/duplo clique.
            process = subprocess.Popen([str(exe)], cwd=ROOT, stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE, text=True)
            header = process.stdout.readline()
            self.assertIn('Literatura 2024', header)
            self.assertIn('Resultado atual', header)
            first = process.stdout.readline()
            self.assertEqual(first.split()[0], '15-r-e')
            rest, errors = process.communicate(timeout=60)
            self.assertEqual(process.returncode, 0, errors)
            self.assertEqual(errors, '')
            lines = (first + rest).splitlines()
            self.assertEqual(len(lines), 60)
            self.assertEqual({line.split()[0] for line in lines}, set(references))
            self.assertTrue(all(len(line.split()) == 7 for line in lines))
            with report.open(newline='') as stream:
                reader = csv.DictReader(stream)
                self.assertEqual(reader.fieldnames, HEADER)
                saved = list(reader)
            self.assertEqual(len(saved), 60)
            for row in saved:
                ref = references[row['instancia']]
                self.assertGreater(float(row['tempo_ultima_execucao_s']), 0)
                self.assertAlmostEqual(float(row['tempo_literatura_2025_s']), float(ref['solver_seconds']))
                value = float(row['melhor_algoritmo'])
                self.assertGreater(value, 0)
                self.assertEqual(float(row['literatura_2024']), float(ref['ub_2024']))
                self.assertEqual(float(row['literatura_2025']), float(ref['ub_2025']))
                expected = 100*(value-float(ref['ub_2025']))/float(ref['ub_2025'])
                self.assertAlmostEqual(float(row['gap_percentual']), expected)
            # Sentinela para testar persistencia; nao e certificado de solucao.
            saved[0]['melhor_algoritmo'] = '1'
            saved[1]['melhor_algoritmo'] = '1000000'
            with report.open('w', newline='') as stream:
                writer = csv.DictWriter(stream, HEADER)
                writer.writeheader()
                writer.writerows(saved)
            second = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True, timeout=60)
            self.assertEqual(second.returncode, 0, second.stderr)
            displayed = second.stdout.splitlines()[1:]
            self.assertEqual(len(displayed), 60)
            # A tela mostra a rodada atual, mesmo com recorde salvo menor.
            self.assertGreater(float(displayed[0].split()[1]), 1)
            current = float(displayed[1].split()[1])
            self.assertGreater(current, 0)
            with report.open(newline='') as stream:
                updated = list(csv.DictReader(stream))
            self.assertEqual(updated[0]['melhor_algoritmo'], '1.000000000')
            self.assertLess(float(updated[1]['melhor_algoritmo']), 1000000)
            self.assertAlmostEqual(float(updated[1]['melhor_algoritmo']), current, places=2)
            for old, new in zip(saved, updated):
                self.assertLessEqual(float(new['melhor_algoritmo']), float(old['melhor_algoritmo']))
            self.assertEqual([p.name for p in (base/'resultados').iterdir()], ['relatorio.csv'])
            # UTF-8 BOM e CRLF devem preservar resultados em qualquer plataforma.
            content = report.read_text(encoding='utf-8')
            for bom, ending in [(b'\xef\xbb\xbf', '\r\n'), (b'', '\n'), (b'', '\r\n')]:
                report.write_bytes(bom + content.replace('\n', ending).encode('utf-8'))
                compatible = subprocess.run([str(exe)], cwd=ROOT, capture_output=True,
                                            text=True, timeout=60)
                self.assertEqual(compatible.returncode, 0, compatible.stderr)
                with report.open(newline='') as stream:
                    self.assertEqual(next(csv.DictReader(stream))['melhor_algoritmo'], '1.000000000')
            report.write_bytes(b'')
            empty = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True, timeout=60)
            self.assertEqual(empty.returncode, 0, empty.stderr)
            with report.open(newline='') as stream:
                self.assertEqual(len(list(csv.DictReader(stream))), 60)
            report.write_text('cabecalho_invalido\n')
            invalid = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True, timeout=30)
            self.assertNotEqual(invalid.returncode, 0)
            self.assertEqual(report.read_text(), 'cabecalho_invalido\n')


if __name__ == '__main__':
    unittest.main()

