"""Impede que o executor misture binarios Windows e Linux, inclusive no WSL."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import executar_benchmark as benchmark


class ExecutavelTests(unittest.TestCase):
    """Assinaturas sinteticas bastam: os arquivos nao sao executados nestes testes."""

    def test_linux_exige_elf_e_ignora_exe_windows(self):
        with tempfile.TemporaryDirectory(dir=benchmark.ROOT) as directory:
            root = Path(directory)
            (root/'pdsvrp_sa.exe').write_bytes(b'MZ\0\0')
            with patch.object(benchmark, 'ROOT', root), \
                 patch.object(benchmark.os, 'name', 'posix'), \
                 patch.object(benchmark.os, 'access', return_value=True):
                with self.assertRaises(FileNotFoundError):
                    benchmark.localizar_executavel()
                (root/'pdsvrp_sa').write_bytes(b'MZ\0\0')
                with self.assertRaisesRegex(RuntimeError, 'nativo'):
                    benchmark.localizar_executavel()
                (root/'pdsvrp_sa').write_bytes(b'\x7fELF')
                self.assertEqual(benchmark.localizar_executavel(), root/'pdsvrp_sa')

    def test_windows_exige_pe(self):
        with tempfile.TemporaryDirectory(dir=benchmark.ROOT) as directory:
            root = Path(directory)
            with patch.object(benchmark, 'ROOT', root), patch.object(benchmark.os, 'name', 'nt'):
                (root/'pdsvrp_sa.exe').write_bytes(b'\x7fELF')
                with self.assertRaisesRegex(RuntimeError, 'nativo'):
                    benchmark.localizar_executavel()
                (root/'pdsvrp_sa.exe').write_bytes(b'MZ\0\0')
                self.assertEqual(benchmark.localizar_executavel(), root/'pdsvrp_sa.exe')
