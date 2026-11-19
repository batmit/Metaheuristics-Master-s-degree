"""Casos adversos: certificados invalidos jamais podem chegar ao registro."""
from copy import deepcopy
import contextlib
import io
import math
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

import executar_benchmark as benchmark


class ValidacaoTests(unittest.TestCase):
    """Dados pequenos permitem conferir duracoes e objetivo manualmente."""

    def setUp(self):
        self.dados = (60.0, 2,
                      ((0, 0, 0, 0), (1, 1, 0, 1), (2, 0, 1, 1), (3, 1, 1, 1)),
                      ((-1, 3), (5, 4), (6, 5)))
        self.cert = dict(routes=[[1, 3], []],
                         missions=[dict(client=2, k=1, start=0, end=5)], makespan=5)
        self.cache = patch.object(benchmark, 'dados_instancia', return_value=self.dados)
        self.cache.start()
        self.addCleanup(self.cache.stop)

    def test_certificado_viavel(self):
        self.assertEqual(benchmark.validar(self.cert, 'teste', 2), 5)

    def test_valida_matriz_experimental_sem_mudar_dados_oficiais(self):
        times=((-1,3),(6,4),(6,5))
        data=(*self.dados[:3],times)
        cert=deepcopy(self.cert)
        cert['missions'][0]['end']=6
        cert['makespan']=6
        self.assertEqual(benchmark.validar(cert,'teste',2,data=data),6)
        with self.assertRaisesRegex(ValueError,'duracao'):
            benchmark.validar(cert,'teste',2)
        self.assertEqual(benchmark.validar(self.cert,'teste',2),5)

    def test_metrica_euclidiana_recalcula_diagonais(self):
        cert = dict(routes=[[1, 2, 3], []], missions=[], makespan=2+2*math.sqrt(2))
        self.assertAlmostEqual(benchmark.validar(cert, 'teste', 2, 'euclidean'), cert['makespan'])
        with self.assertRaisesRegex(ValueError, 'makespan'):
            benchmark.validar(cert, 'teste', 2)
        with self.assertRaisesRegex(ValueError, 'Metrica'):
            benchmark.validar(cert, 'teste', 2, 'desconhecida')

    def test_rejeita_cobertura_e_indices(self):
        for routes in ([[1], []], [[1, 1, 3], []], [[0, 1, 3], []],
                       [[-1, 3], []], [[4, 3], []], [[True, 3], []],
                       [[1.0, 3], []], [[1, 3]], [[1, 2, 3], []]):
            with self.subTest(routes=routes):
                cert = dict(self.cert, routes=routes)
                with self.assertRaises(ValueError):
                    benchmark.validar(cert, 'teste', 2)

    def test_rejeita_dados_de_missao(self):
        variants = [('client', 0), ('client', 4), ('client', True),
                    ('k', 0), ('k', 3), ('k', 1.5), ('k', True),
                    ('start', -1), ('start', math.nan), ('end', math.inf),
                    ('end', -2), ('end', 4), ('start', '0')]
        for field, value in variants:
            with self.subTest(field=field, value=value):
                cert = deepcopy(self.cert)
                cert['missions'][0][field] = value
                with self.assertRaises(ValueError):
                    benchmark.validar(cert, 'teste', 2)

    def test_rejeita_par_cliente_k_inviavel(self):
        cert = dict(routes=[[2, 3], []],
                    missions=[dict(client=1, k=1, start=0, end=3)], makespan=4)
        with self.assertRaisesRegex(ValueError, 'inviavel'):
            benchmark.validar(cert, 'teste', 2)

    def test_rejeita_excesso_de_drones(self):
        cert = dict(routes=[[3], []], missions=[dict(client=1, k=2, start=0, end=3),
                                               dict(client=2, k=1, start=0, end=5)], makespan=5)
        with self.assertRaisesRegex(ValueError, 'capacidade'):
            benchmark.validar(cert, 'teste', 2)

    def test_permite_reutilizar_drone_no_instante_do_retorno(self):
        cert = dict(routes=[[3], []], missions=[dict(client=1, k=2, start=0, end=3),
                                               dict(client=2, k=1, start=3, end=8)], makespan=8)
        self.assertEqual(benchmark.validar(cert, 'teste', 2), 8)

    def test_rejeita_objetivo_incorreto_e_estrutura(self):
        for cert in (None, {}, dict(self.cert, missions=[{}]),
                     dict(self.cert, routes=None), dict(self.cert, missions=None),
                     *(dict(self.cert, makespan=v) for v in (0, 4, -1, math.nan, math.inf, True))):
            with self.subTest(cert=cert):
                with self.assertRaises(ValueError):
                    benchmark.validar(cert, 'teste', 2)


class IntegracaoTests(unittest.TestCase):
    """Verifica registro real, modo otimizado e contrato de saida do programa."""

    def test_limite_local_detecta_referencia_impossivel_126(self):
        bound, client = benchmark.limite_inferior_local('126-rc-e')
        self.assertEqual((bound, client), (112, 46))
        registro = benchmark.Registro()
        incompatible = {key for key, entry in registro.data.items()
                        if not entry['literature_comparable']}
        self.assertEqual(incompatible, {'126-rc-e_s3', '126-rc-e_s4', '126-rc-e_s5'})
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            benchmark.mostrar(dict(registro.data['126-rc-e_s5'], best={'makespan': 112}))
        self.assertIn('N/C', output.getvalue())
        self.assertNotIn('47.368', output.getvalue())

    def test_gap_percentual_real_nao_e_ocultado(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            benchmark.mostrar(dict(instance='68-rc-c', trucks=5, literature_ub=44,
                                   best={'makespan': 56}, runs=[]))
        self.assertIn('27.273', output.getvalue())

    def test_todos_os_certificados_salvos(self):
        registro = benchmark.Registro()
        self.assertEqual(len(registro.data), len(benchmark.catalogo()))
        for entry in registro.data.values():
            if entry['best'] is not None:
                benchmark.validar(entry['best'], entry['instance'], entry['trucks'])

    def test_python_otimizado_mantem_a_validacao(self):
        # O certificado vazio deve falhar mesmo com a remocao de asserts pelo -O.
        code = ('from executar_benchmark import validar\n'
                'try:\n validar({}, "50-r-e", 2)\n'
                'except ValueError:\n pass\n'
                'else:\n raise RuntimeError("validacao desativada")\n')
        subprocess.run([sys.executable, '-O', '-c', code], cwd=Path(__file__).parent,
                       check=True, capture_output=True, text=True)

    def test_list_imprime_apenas_uma_tabela(self):
        output = io.StringIO()
        with patch.object(sys, 'argv', ['executar_benchmark.py', '--list']), contextlib.redirect_stdout(output):
            benchmark.main()
        lines = output.getvalue().splitlines()
        self.assertEqual(len(lines), len(benchmark.catalogo()) + 1)
        self.assertTrue(lines[0].startswith('Instancia'))
        self.assertIn('Execucao (s)', lines[0])
        self.assertNotIn('Estado', lines[0])
        self.assertNotIn('Literatura (s)', lines[0])
        for line in lines[1:]:
            self.assertRegex(line, r'^\S+ \(S=\d+\)\s+\d+\.\d{6}\s+(--|\d+\.\d{6})\s+(N/C|--|-?\d+\.\d{3})\s+(--|\d+\.\d{3})$')


if __name__ == '__main__':
    unittest.main()
