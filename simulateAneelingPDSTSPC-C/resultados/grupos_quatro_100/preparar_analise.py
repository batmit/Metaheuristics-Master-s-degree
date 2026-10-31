"""Reutiliza o validador independente, adaptando apenas os metadados do lote."""
from pathlib import Path

HERE=Path(__file__).resolve().parent
source=(HERE.parent/'experimento_melhorias_67/analisar.py').read_text()
source=source.replace("stage=HERE/sys.argv[1]",'stage=HERE')
source=source.replace("range(manifest['seed'],manifest['seed']+len(rows))", "range(manifest['seed_by_instance'][key[0]],manifest['seed_by_instance'][key[0]]+len(rows))")
source=source.replace("assert 'finished' in manifest", "assert 'finished' in manifest\nassert manifest['sources_unchanged']\nassert len(manifest['returncodes'])==160 and all(t['code']==0 for t in manifest['returncodes'])")
source=source.replace("print(json.dumps(summary,ensure_ascii=False,indent=2));raise SystemExit()", "print(json.dumps([{'instance':r['instance'],'mask':r['mask'],'n':r['n'],'best':r['best'],'mean':r['mean']} for r in summary],ensure_ascii=False));raise SystemExit()")
(HERE/'analisar.py').write_text(source)
print('Validador preparado.')
