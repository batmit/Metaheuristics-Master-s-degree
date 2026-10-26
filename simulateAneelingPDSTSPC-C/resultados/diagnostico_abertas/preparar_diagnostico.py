"""Cria copias instrumentadas, sem editar SA.c/cv.c ou consumir novos sorteios."""
from pathlib import Path
import json

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
src=(ROOT/'SA.c').read_text()

prefix=r'''
#include <stdio.h>
#include <time.h>
typedef struct Diag {
    unsigned long long attempts[NUM_MOVIMENTOS], accepted[NUM_MOVIMENTOS];
    unsigned long long neutral[NUM_MOVIMENTOS], secondary[NUM_MOVIMENTOS];
    unsigned long long lower[NUM_MOVIMENTOS], uphill[NUM_MOVIMENTOS];
    unsigned long long late[NUM_MOVIMENTOS], late_neutral[NUM_MOVIMENTOS];
    unsigned long long global[NUM_MOVIMENTOS], statuses[4], nodes;
    unsigned long long step, last_improvement, equal_rejects;
    unsigned long long bb_triggers, bb_applied, bb_global_gains;
    unsigned long long drone_dominant, truck_dominant, tied;
    double bb_seconds;
} Diag;
static Diag diag;
static int diag_same_assignment(const Solucao *a,const Solucao *b) {
    if(a->numMissoesDrone!=b->numMissoesDrone)return 0;
    for(int i=0;i<a->numMissoesDrone;i++) {
        int found=0;
        for(int j=0;j<b->numMissoesDrone;j++)
            if(a->missaoDrone[i].cliente==b->missaoDrone[j].cliente) {found=1;break;}
        if(!found)return 0;
    }
    return 1;
}
static void diag_array(const char *key,const unsigned long long *values) {
    printf(",\"%s\":[",key);
    for(int i=0;i<NUM_MOVIMENTOS;i++)printf("%s%llu",i?",":"",values[i]);
    printf("]");
}
static void diag_print(const Solucao *elite,int n,const PesosMovimentosSA *weights,
                       const HistoricoBBSA *history) {
    int unique=0;
    for(int i=0;i<n;i++) {
        int duplicate=0;
        for(int j=0;j<i;j++)if(diag_same_assignment(&elite[i],&elite[j]))duplicate=1;
        if(!duplicate)unique++;
    }
    printf("DIAG {\"steps\":%llu,\"last_improvement\":%llu,\"equal_elite_rejections\":%llu,"
           "\"bb_triggers\":%llu,\"bb_applied\":%llu,\"bb_global_gains\":%llu,"
           "\"bb_executions\":%llu,\"bb_reuses\":%llu,\"bb_nodes\":%llu,\"bb_seconds\":%.6f,"
           "\"bb_status\":[%llu,%llu,%llu,%llu],\"final_elite_size\":%d,\"unique_elite_assignments\":%d,"
           "\"truck_dominant\":%llu,\"drone_dominant\":%llu,\"tied\":%llu",
           diag.step,diag.last_improvement,diag.equal_rejects,diag.bb_triggers,
           diag.bb_applied,diag.bb_global_gains,history->execucoes,history->reutilizacoes,
           diag.nodes,diag.bb_seconds,diag.statuses[0],diag.statuses[1],diag.statuses[2],
           diag.statuses[3],n,unique,diag.truck_dominant,diag.drone_dominant,diag.tied);
    diag_array("attempts",diag.attempts);diag_array("accepted",diag.accepted);
    diag_array("neutral",diag.neutral);diag_array("secondary",diag.secondary);
    diag_array("lower",diag.lower);diag_array("uphill",diag.uphill);
    diag_array("late",diag.late);diag_array("late_neutral",diag.late_neutral);
    diag_array("global",diag.global);
    printf(",\"final_weights\":[");
    for(int i=0;i<NUM_MOVIMENTOS;i++)printf("%s%.9f",i?",":"",weights->pesos[i]);
    puts("]}");fflush(stdout);
}
'''
src=src.replace('#include <string.h>','#include <string.h>\n'+prefix,1)
src=src.replace('if (candidata->makespan >= melhores[destino].makespan) return 0;',
    'if (candidata->makespan >= melhores[destino].makespan) { if(candidata->makespan == melhores[destino].makespan)diag.equal_rejects++; return 0; }')
src=src.replace('        ResultadoBB resultado;','        ResultadoBB resultado;\n        clock_t diag_bb_start=clock();',1)
needle='                                               maxNos,refinada.tempoCaminhao,&resultado);'
assert needle in src
src=src.replace(needle,needle+'\n        diag.bb_seconds+=(double)(clock()-diag_bb_start)/CLOCKS_PER_SEC;\n        diag.nodes+=resultado.nos;diag.statuses[resultado.motivo]++;',1)
needle='int simulatedAnnealing(const Instancia *inst, Solucao *solucao, const ParametrosSA *parametros){'
assert needle in src
src=src.replace(needle,needle+'\n    memset(&diag,0,sizeof diag);',1)
needle='            double delta = candidata.makespan - atual.makespan;'
assert src.count(needle)==1
src=src.replace(needle,needle+r'''
            diag.step++;
            if(atual.tempoCaminhao>atual.tempoDrones)diag.truck_dominant++;
            else if(atual.tempoDrones>atual.tempoCaminhao)diag.drone_dominant++;
            else diag.tied++;
''')
needle='            if (parametros->pesosAdaptativos) {'
assert src.count(needle)==1
src=src.replace(needle,r'''
            diag.attempts[movimento]++;diag.accepted[movimento]+=aceitar;
            diag.neutral[movimento]+=delta==0.0;
            diag.lower[movimento]+=delta<0.0;
            diag.uphill[movimento]+=aceitar && delta>0.0;
            diag.secondary[movimento]+=delta==0.0 &&
                candidata.tempoCaminhao+candidata.tempoDrones<atual.tempoCaminhao+atual.tempoDrones;
            if(temperatura<=1.0) {
                diag.late[movimento]++;diag.late_neutral[movimento]+=delta==0.0;
            }
            if(candidata.makespan<melhores[0].makespan) {
                diag.global[movimento]++;diag.last_improvement=diag.step;
            }
'''+needle)
src=src.replace('            double antes = melhores[0].makespan;',
                '            diag.bb_triggers++;\n            double antes = melhores[0].makespan;',1)
src=src.replace('            if (refinou < 0) goto finalizar;',
                '            if (refinou < 0) goto finalizar;\n            diag.bb_applied+=refinou>0;\n            if(melhores[0].makespan<antes) {diag.bb_global_gains++;diag.last_improvement=diag.step;}',1)
src=src.replace('    copiarSolucao(inst, &melhores[0], solucao);',
                '    diag_print(melhores,quantidadeMelhores,&pesos,&historicoBB);\n    copiarSolucao(inst, &melhores[0], solucao);',1)
(HERE/'SA_instrumentado.c').write_text(src,encoding='utf-8')

cases=[('20-c-c','small',123),('67-c-c','large',1792859838),
       ('105-rc-e','large',1792860329),('154-c-c','large',1792860948),
       ('165-r-c','large',1792861080),('167-r-e','large',1792861155),
       ('181-r-e','large',123)]
witnesses=json.loads((ROOT/'resultados/revisao_literatura_2025/testemunhas_validadas.json').read_text())
text=r'''
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "SA.h"
#include "lerInstancias.h"
static void dump(const char *type,const char *name,const Solucao *s) {
    printf("%s %s %.9f %.9f %.9f ROUTE",type,name,s->makespan,s->tempoCaminhao,s->tempoDrones);
    for(int i=0;i<s->numMissoesCaminhao;i++)printf(" %d",s->missaoCaminhao[i].n);
    printf(" MISSIONS");
    for(int i=0;i<s->numMissoesDrone;i++) {MissaoDrone m=s->missaoDrone[i];
        printf(" %d:%d:%.9f:%.9f",m.cliente,m.qtdDrones,m.tempoInicio,m.tempoFim);}
    puts("");fflush(stdout);
}
static void run(const char *name,const char *dir,unsigned int seed,int nc,const int *route,
                int nd,const int (*mis)[2],double target) {
    char path[256];Instancia in={0};Solucao s={0};
    snprintf(path,sizeof path,"instances/pdstsp-c/%s/%s.txt",dir,name);
    assert(lerInstancia(path,&in));alocarInstancias(&in);construirMatrizCaminhao(&in);
    snprintf(path,sizeof path,"instances/pdstsp-c/%s/tau_kj_%s.txt",dir,name);
    assert(lerTemposDrone(path,&in));alocarSolucao(&in,&s);
    s.numMissoesCaminhao=nc;s.numMissoesDrone=nd;
    for(int i=0;i<nc;i++)s.missaoCaminhao[i].n=route[i];
    for(int i=0;i<nd;i++)s.missaoDrone[i]=(MissaoDrone){mis[i][0],mis[i][1],0,0};
    assert(escalonarMissoesDrone(&in,&s)&&calcularMakespan(&in,&s));
    assert(s.makespan<=target);dump("REFERENCE_IN_C",name,&s);
    assert(construirSolucaoInicial(&in,&s));double initial=s.makespan;
    srand(seed);ParametrosSA p=parametrosSAPadrao();double t;clock_t start=clock();
    assert(calculaTemperaturaInicial(&in,&s,&p,&t));p.tempInicial=t;
    printf("START %s %u %.9f %.9f\n",name,seed,initial,t);fflush(stdout);
    assert(simulatedAnnealing(&in,&s,&p));assert(calcularMakespan(&in,&s));
    dump("SA_FINAL",name,&s);
    printf("TIME %s %.6f\n",name,(double)(clock()-start)/CLOCKS_PER_SEC);fflush(stdout);
    liberarSolucao(&s);liberarInstancia(&in);
}
int main(void) {
'''
for idx,(name,directory,seed) in enumerate(cases):
    w=witnesses[name];route=w['route'][1:-1]
    mis=sorted(w['missions'],key=lambda x:(x['start'],x['client']))
    text+=f'    static const int r{idx}[]={{'+','.join(map(str,route))+'};\n'
    text+=f'    static const int d{idx}[][2]={{'+','.join('{'+f'{m["client"]},{m["k"]}'+'}' for m in mis)+'};\n'
    text+=f'    run("{name}","{directory}",{seed}u,{len(route)},r{idx},{len(mis)},d{idx},{w["makespan"]});\n'
text+='    return 0;\n}\n'
(HERE/'executar_diagnostico.c').write_text(text,encoding='utf-8')
print('Copias instrumentadas e teste de representacao preparados; SA.c/cv.c intactos.')
