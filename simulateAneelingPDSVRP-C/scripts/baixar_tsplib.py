"""Download a fixed, preselected EUC_2D panel and its official optimal tours."""
import gzip
import hashlib
import json
from pathlib import Path
from urllib.request import urlopen
from urllib.error import HTTPError

ROOT=Path(__file__).resolve().parents[1]
BASE='https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/'
OPTIMA={'berlin52':7542,'st70':675,'kroA100':21282,'eil101':629,
        'lin105':14379,'ch130':6110,'ch150':6528,'a280':2579}

def main():
    out=ROOT/'instances/tsplib';out.mkdir(exist_ok=True)
    manifest={}
    for name,optimum in OPTIMA.items():
        files=[]
        for suffix in ['.tsp.gz','.opt.tour' if name=='st70' else '.opt.tour.gz']:
            filename=name+suffix
            path=out/filename
            if not path.exists():
                try:
                    with urlopen(BASE+filename,timeout=45) as response:
                        data=response.read()
                except HTTPError as e:
                    if e.code==404 and suffix=='.opt.tour':
                        suffix+='.gz';filename=name+suffix;path=out/filename
                        with urlopen(BASE+filename,timeout=45) as response:
                            data=response.read()
                    else: raise
                if suffix.endswith('.gz'): gzip.decompress(data)
                path.write_bytes(data)
            raw=path.read_bytes()
            unpacked=gzip.decompress(raw) if suffix.endswith('.gz') else raw
            (out/(filename[:-3] if suffix.endswith('.gz') else filename)).write_bytes(unpacked)
            files.append(dict(url=BASE+filename,sha256=hashlib.sha256(raw).hexdigest()))
        manifest[name]=dict(optimum=optimum,files=files)
        print(name, 'downloaded',flush=True)
    with urlopen(BASE+'TSP-BEST.html',timeout=45) as response:
        (out/'TSP-BEST.html').write_bytes(response.read())
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

if __name__=='__main__': main()
