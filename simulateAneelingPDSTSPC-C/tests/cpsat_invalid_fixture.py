"""Certificado deliberadamente invalido para testar a barreira Python -> C."""
import pathlib
import sys
pathlib.Path(sys.argv[2]).write_text("FEASIBLE\n1 1\n2\n2 1 0 8\n", encoding="ascii")
