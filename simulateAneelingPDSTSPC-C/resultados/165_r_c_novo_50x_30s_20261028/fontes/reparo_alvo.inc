/* Incluido uma unica vez por cv.c; utiliza seus auxiliares privados. */
#include <time.h>
#include <limits.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <sys/stat.h>
#endif

static double relogioReparoCV(void)
{
#ifdef _WIN32
    LARGE_INTEGER c,f;QueryPerformanceCounter(&c);QueryPerformanceFrequency(&f);
    return (double)c.QuadPart/(double)f.QuadPart;
#else
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec+1e-9*t.tv_nsec;
#endif
}

int metasReparoCV(const Instancia *in,double melhor,double *h,double *rota)
{
    if(!in||!h||!rota||!isfinite(melhor)||melhor<=0||in->n<1||
       !in->tempoCaminhao||!in->tempoDrone||!in->clientes) return 0;
    /* Nao presume integralidade em instancias externas. */
    bool manhattan=in->truckSpeed==30.0;
    for(int i=0;i<in->n;i++) {
        if(!in->tempoCaminhao[i]||!in->tempoDrone[i]) return 0;
        if(fabs(in->clientes[i].x-round(in->clientes[i].x))>1e-10||
           fabs(in->clientes[i].y-round(in->clientes[i].y))>1e-10) manhattan=false;
        for(int j=0;j<in->n;j++) {
            double t=in->tempoCaminhao[i][j];
            if(!isfinite(t)||t<0||t>INT_MAX||fabs(t-round(t))>1e-10) return 0;
            double m=2*(fabs(in->clientes[i].x-in->clientes[j].x)+fabs(in->clientes[i].y-in->clientes[j].y));
            if(fabs(t-m)>1e-10) manhattan=false;
        }
        if(i)for(int k=1;k<=in->numDrones;k++) {
            double t=in->tempoDrone[i][k];
            if(isfinite(t)&&t>=0&&(t>INT_MAX||fabs(t-round(t))>1e-10)) return 0;
        }
    }
    *h=ceil(melhor)-1;*rota=manhattan?4*floor(*h/4):*h;
    return *h>=0;
}

double limiteCargaReparoCV(const Instancia *in,const Solucao *s)
{
    if(!in||!s||in->numDrones<0) return INFINITY;
    long double area=0;
    for(int i=0;i<s->numMissoesDrone;i++) {
        int j=s->missaoDrone[i].cliente;
        if(j<=0||j>=in->n) return INFINITY;
        double v=cargaMinimaClienteCV(in,j);if(!isfinite(v))return INFINITY;area+=v;
    }
    if(!s->numMissoesDrone)return 0;
    return in->numDrones? (double)(area/in->numDrones):INFINITY;
}

/* Minima 1-tree sem potenciais. min(t_ij,t_ji) tambem e seguro em matrizes
 * assimetricas. O bound nao confunde o custo da rota atual com um limite. */
double limiteRotaReparoCV(const Instancia *in,const Solucao *s)
{
    if(!in||!s||s->numMissoesCaminhao<0)return INFINITY;
    int n=s->numMissoesCaminhao+1;
    if(n==1)return 0;
    int *ids=malloc((size_t)n*sizeof *ids);
    double *key=malloc((size_t)n*sizeof *key);
    unsigned char *used=calloc((size_t)n,1);
    if(!ids||!key||!used){free(ids);free(key);free(used);return 0;}
    ids[0]=0;
    for(int i=1;i<n;i++)ids[i]=s->missaoCaminhao[i-1].n;
    if(n==2){double v=in->tempoCaminhao[0][ids[1]]+in->tempoCaminhao[ids[1]][0];free(ids);free(key);free(used);return v;}
    for(int i=0;i<n;i++)key[i]=INFINITY;
    key[1]=0;long double total=0;double first=INFINITY,second=INFINITY;
    for(int pass=1;pass<n;pass++) {
        int j=-1;for(int k=1;k<n;k++)if(!used[k]&&(j<0||key[k]<key[j]))j=k;
        used[j]=1;total+=key[j];
        for(int k=1;k<n;k++)if(!used[k]) {
            double v=fmin(in->tempoCaminhao[ids[j]][ids[k]],in->tempoCaminhao[ids[k]][ids[j]]);
            if(v<key[k])key[k]=v;
        }
        double v=fmin(in->tempoCaminhao[0][ids[j]],in->tempoCaminhao[ids[j]][0]);
        if(v<first){second=first;first=v;}else if(v<second)second=v;
    }
    free(ids);free(key);free(used);return (double)(total+first+second);
}

static void copiarCompletaReparoCV(const Instancia *in,const Solucao *a,Solucao *b)
{
    copiarDecisoesCV(a,b);b->tempoCaminhao=a->tempoCaminhao;b->tempoDrones=a->tempoDrones;
    b->makespan=a->makespan;b->avaliacaoValida=a->avaliacaoValida;b->cronogramaValido=a->cronogramaValido;
    b->cronogramaExterno=a->cronogramaExterno;
    if(a->prefixosDrone&&b->prefixosDrone)memcpy(b->prefixosDrone,a->prefixosDrone,((size_t)a->numMissoesDrone+1)*sizeof(double));
    (void)in;
}

static double economiaReparoCV(const Instancia *in,const Solucao *s,int pos)
{
    int j=s->missaoCaminhao[pos].n,a=pos?s->missaoCaminhao[pos-1].n:0;
    int b=pos+1<s->numMissoesCaminhao?s->missaoCaminhao[pos+1].n:0;
    return tempoTrecho(in,a,j)+tempoTrecho(in,j,b)-tempoTrecho(in,a,b);
}

int selecionarLivresReparoCV(const Instancia *in,const Solucao *s,int *out,int cap)
{
    if(!in||!s||!out||cap<1)return 0;
    unsigned char *used=calloc((size_t)in->n,1);if(!used)return 0;
    int n=0,ancora=0;
    for(int fase=0;fase<2;fase++)for(int h=0;h<cap/2;h++) {
        int j=-1;double best=-INFINITY;
        int total=fase?s->numMissoesDrone:s->numMissoesCaminhao;
        for(int i=0;i<total;i++) {
            int x=fase?s->missaoDrone[i].cliente:s->missaoCaminhao[i].n;
            if(used[x]||!clientePodeUsarDrone(in,x))continue;
            double delta=0,v;
            if(fase){melhorPosicaoCaminhaoCV(in,s,x,&delta);v=cargaMinimaClienteCV(in,x)/(1+fmax(0,delta));}
            else v=10*economiaReparoCV(in,s,i)+1/(1+cargaMinimaClienteCV(in,x));
            /* Pequeno desempate aleatorio diversifica vizinhancas repetidas. */
            v+=1e-5*(double)rand()/RAND_MAX;
            if(v>best){best=v;j=x;}
        }
        if(j<0)break;
        if(!ancora)ancora=j;
        used[j]=1;out[n++]=j;
    }
    while(n<cap) {
        int j=-1;double best=INFINITY;
        for(int x=1;x<in->n;x++)if(!used[x]&&clientePodeUsarDrone(in,x)) {
            double v=fabs(in->clientes[x].x-in->clientes[ancora].x)+fabs(in->clientes[x].y-in->clientes[ancora].y);
            if(v<best){best=v;j=x;}
        }
        if(j<0)break;
        used[j]=1;out[n++]=j;
    }
    free(used);return n;
}

typedef struct EstadoCadeiaCV {Solucao s;unsigned char *usados;double nota;unsigned long long hash;} EstadoCadeiaCV;
static unsigned long long hashAtribuicaoReparoCV(const Solucao *s)
{
    unsigned long long h=0;
    for(int i=0;i<s->numMissoesDrone;i++) {unsigned long long x=(unsigned)s->missaoDrone[i].cliente+0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;h^=x^(x>>31);}
    return h;
}
static int alocarEstadoReparoCV(const Instancia *in,EstadoCadeiaCV *e)
{
    Instancia dim={.n=in->n,.numDrones=in->numDrones};alocarSolucao(&dim,&e->s);
    e->usados=calloc((size_t)in->n,1);
    return e->s.missaoCaminhao&&e->s.missaoDrone&&e->s.prefixosDrone&&e->usados;
}
static void guardarEstadoReparoCV(const Instancia *in,const Solucao *s,const unsigned char *used,double nota,
                                 EstadoCadeiaCV *pool,int *q,int width)
{
    unsigned long long hash=hashAtribuicaoReparoCV(s);
    int dest=*q;
    for(int i=0;i<*q;i++)if(pool[i].hash==hash) {if(pool[i].nota<=nota)return;dest=i;break;}
    if(dest==*q&&*q==width){dest=0;for(int i=1;i<*q;i++)if(pool[i].nota>pool[dest].nota)dest=i;if(nota>=pool[dest].nota)return;}
    if(dest==*q)(*q)++;
    copiarDecisoesCV(s,&pool[dest].s);memcpy(pool[dest].usados,used,(size_t)in->n);
    pool[dest].nota=nota;pool[dest].hash=hash;
}
/* Repara prioridades/quantidades somente depois de fechar a cadeia. */
static int escalonarAlvoReparoCV(const Instancia *in,Solucao *s,double h,double tr,
                                double fim,Solucao *work,MetricasReparoCV *m)
{
    if(custoRotaMelhoria(in,s)>tr+1e-8)return 0;
    for(int order=0;order<4&&relogioReparoCV()<fim;order++) {
        copiarDecisoesCV(s,work);
        for(int i=0;i<work->numMissoesDrone;i++)work->missaoDrone[i].qtdDrones=menorCargaKCV(in,work->missaoDrone[i].cliente);
        if(order)for(int i=1;i<work->numMissoesDrone;i++) {
            MissaoDrone x=work->missaoDrone[i];double vx=order==1?in->tempoDrone[x.cliente][x.qtdDrones]:order==2?x.qtdDrones:cargaMinimaClienteCV(in,x.cliente);int j=i;
            while(j>0){MissaoDrone y=work->missaoDrone[j-1];double vy=order==1?in->tempoDrone[y.cliente][y.qtdDrones]:order==2?y.qtdDrones:cargaMinimaClienteCV(in,y.cliente);if(vy>=vx)break;work->missaoDrone[j]=y;j--;}
            work->missaoDrone[j]=x;
        }
        m->avaliacoes++;
        if(!escalonarMissoesDrone(in,work)||!calcularMakespan(in,work))return -1;
        if(work->makespan<=h+1e-9){copiarCompletaReparoCV(in,work,s);return 1;}
    }
    return 0;
}

int repararComMetaCV(const Instancia *in,const Solucao *base,double global,Solucao *out,
                    bool chains,bool filters,int width,int depth,double seconds,MetricasReparoCV *stats)
{
    if(!in||!base||!out||base==out||base->missaoCaminhao==out->missaoCaminhao||
       base->missaoDrone==out->missaoDrone||!base->avaliacaoValida||!out->missaoCaminhao||
       !out->missaoDrone||!out->prefixosDrone||width<1||width>16||depth<1||depth>6||!isfinite(seconds)||seconds<=0)return -1;
    MetricasReparoCV local={0};MetricasReparoCV *m=stats?stats:&local;
    double start=relogioReparoCV(),h,tr;if(!metasReparoCV(in,global,&h,&tr))return 0;
    double end=start+seconds;EstadoCadeiaCV *a=calloc((size_t)width,sizeof *a),*b=calloc((size_t)width,sizeof *b);
    Solucao test={0},work={0};Instancia dim={.n=in->n,.numDrones=in->numDrones};int result=-1;
    int *choices=malloc((size_t)in->n*sizeof *choices);unsigned char *used=calloc((size_t)in->n,1);
    alocarSolucao(&dim,&test);alocarSolucao(&dim,&work);
    if(!a||!b||!choices||!used||!test.missaoCaminhao||!test.missaoDrone||!test.prefixosDrone||!work.missaoCaminhao||!work.missaoDrone||!work.prefixosDrone)goto done;
    for(int i=0;i<width;i++)if(!alocarEstadoReparoCV(in,&a[i])||!alocarEstadoReparoCV(in,&b[i]))goto done;
    copiarDecisoesCV(base,&a[0].s);int na=1;result=0;
    for(int level=0;level<(chains?depth:1)&&na&&relogioReparoCV()<end;level++) {
        int nb=0;
        for(int q=0;q<na&&relogioReparoCV()<end;q++) {
            Solucao *s=&a[q].s;int nc=selecionarLivresReparoCV(in,s,choices,in->n<17?in->n-1:16);
            for(int opt=0;opt<nc&&relogioReparoCV()<end;opt++) {
                int j=choices[opt];if(a[q].usados[j])continue;
                copiarDecisoesCV(s,&test);memcpy(used,a[q].usados,(size_t)in->n);int pos=-1;
                for(int i=0;i<test.numMissoesCaminhao;i++)if(test.missaoCaminhao[i].n==j)pos=i;
                if(pos>=0) {
                    /* Alterna cliente e local inteiro para expor economias ocultas. */
                    bool group=opt%2==0;int n=0;
                    for(int i=0;i<test.numMissoesCaminhao;i++) {
                        int x=test.missaoCaminhao[i].n;
                        bool take=x==j||(group&&!used[x]&&clientePodeUsarDrone(in,x)&&in->clientes[x].x==in->clientes[j].x&&in->clientes[x].y==in->clientes[j].y);
                        if(take){int k=menorCargaKCV(in,x);if(!k)goto done;test.missaoDrone[test.numMissoesDrone++]=(MissaoDrone){x,k,0,0};used[x]=1;}
                        else test.missaoCaminhao[n++]=test.missaoCaminhao[i];
                    }
                    test.numMissoesCaminhao=n;
                } else {
                    int idx=-1;for(int i=0;i<test.numMissoesDrone;i++)if(test.missaoDrone[i].cliente==j)idx=i;
                    if(idx<0)goto done;
                    memmove(test.missaoDrone+idx,test.missaoDrone+idx+1,(size_t)(test.numMissoesDrone-idx-1)*sizeof(MissaoDrone));test.numMissoesDrone--;
                    double delta;int p=melhorPosicaoCaminhaoCV(in,&test,j,&delta);inserirCaminhaoCV(&test,j,p);used[j]=1;
                }
                m->propostas++;polirRotaReconstrucaoCV(in,&test);
                double route=custoRotaMelhoria(in,&test),load=limiteCargaReparoCV(in,&test);
                double note=fmax(0,route-tr)+fmax(0,load-h)+.001*(route+load);
                /* Estados intermediarios inviaveis para a meta continuam no beam:
                 * a proxima transferencia pode compensar o excesso. */
                guardarEstadoReparoCV(in,&test,used,note,b,&nb,width);
                if(filters&&load>h+1e-8){m->podasCarga++;continue;}
                if(filters&&limiteRotaReparoCV(in,&test)>tr+1e-8){m->podasRota++;continue;}
                int found=escalonarAlvoReparoCV(in,&test,h,tr,end,&work,m);
                if(found<0){result=-1;goto done;}
                if(found){copiarCompletaReparoCV(in,&test,out);m->melhorias++;result=1;goto done;}
            }
        }
        EstadoCadeiaCV *swap=a;a=b;b=swap;na=nb;
    }
done:
    if(a)for(int i=0;i<width;i++){liberarSolucao(&a[i].s);free(a[i].usados);}
    if(b)for(int i=0;i<width;i++){liberarSolucao(&b[i].s);free(b[i].usados);}
    free(a);free(b);free(choices);free(used);liberarSolucao(&test);liberarSolucao(&work);
    m->segundosNativo+=relogioReparoCV()-start;return result;
}

static int processoCPSATCV(const char *python,const char *script,const char *request,const char *response,double seconds)
{
#ifdef _WIN32
    /* Sem shell: argumentos sao caminhos, nunca trechos de comando. Aspas
     * internas nao sao caminhos Windows validos e sao rejeitadas. */
    if(strchr(python,'"')||strchr(script,'"')||strchr(request,'"')||strchr(response,'"'))return -1;
    char cmd[4096];int n=snprintf(cmd,sizeof cmd,"\"%s\" \"%s\" \"%s\" \"%s\"",python,script,request,response);
    if(n<0||(size_t)n>=sizeof cmd)return -1;
    STARTUPINFOA si={0};PROCESS_INFORMATION pi={0};si.cb=sizeof si;
    if(!CreateProcessA(NULL,cmd,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi))return -1;
    DWORD wait=WaitForSingleObject(pi.hProcess,(DWORD)ceil(seconds*1000));int result=0;
    if(wait==WAIT_TIMEOUT){TerminateProcess(pi.hProcess,124);WaitForSingleObject(pi.hProcess,INFINITE);result=0;}
    else if(wait!=WAIT_OBJECT_0){TerminateProcess(pi.hProcess,125);WaitForSingleObject(pi.hProcess,INFINITE);result=-1;}
    else {DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);result=code==0?1:-1;}
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return result;
#else
    pid_t pid=fork();if(pid<0)return -1;
    if(!pid){execlp(python,python,script,request,response,(char *)NULL);_exit(127);}
    double until=relogioReparoCV()+seconds;int status;
    while(1){pid_t r=waitpid(pid,&status,WNOHANG);if(r==pid)return WIFEXITED(status)&&WEXITSTATUS(status)==0?1:-1;if(r<0)return -1;if(relogioReparoCV()>=until){kill(pid,SIGKILL);waitpid(pid,&status,0);return 0;}poll(NULL,0,1);}
#endif
}

int repararCPSATCV(const Instancia *in,const Solucao *base,double global,Solucao *out,int cap,
                  double seconds,unsigned seed,const char *python,const char *script,MetricasReparoCV *stats)
{
    if(!in||!base||!out||base==out||base->missaoCaminhao==out->missaoCaminhao||
       base->missaoDrone==out->missaoDrone||!base->avaliacaoValida||cap<1||cap>64||
       !isfinite(seconds)||seconds<=0||seconds>3600||!python||!*python||!script||!*script||
       !out->missaoCaminhao||!out->missaoDrone||!out->prefixosDrone)return -1;
    MetricasReparoCV local={0};MetricasReparoCV *m=stats?stats:&local;
    double start=relogioReparoCV(),h,tr;int result=-1;
    if(!metasReparoCV(in,global,&h,&tr))return 0;
    int *freeids=malloc((size_t)cap*sizeof *freeids);char req[1024]={0},res[1024]={0};FILE *f=NULL;
    Solucao candidate={0};Instancia dim={.n=in->n,.numDrones=in->numDrones};alocarSolucao(&dim,&candidate);
    if(!freeids||!candidate.missaoCaminhao||!candidate.missaoDrone||!candidate.prefixosDrone)goto cleanup;
    int count=selecionarLivresReparoCV(in,base,freeids,cap);if(!count){result=0;goto cleanup;}
#ifdef _WIN32
    if(!CreateDirectoryA("tmp",NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)goto cleanup;
    if(!GetTempFileNameA("tmp","cpr",0,req)||!GetTempFileNameA("tmp","cpo",0,res))goto cleanup;
#else
    if(mkdir("tmp",0700)&&errno!=EEXIST)goto cleanup;
    strcpy(req,"tmp/cprXXXXXX");strcpy(res,"tmp/cpoXXXXXX");
    int a=mkstemp(req);if(a<0)goto cleanup;close(a);int b=mkstemp(res);if(b<0)goto cleanup;close(b);
#endif
    f=fopen(req,"w");if(!f)goto cleanup;
    fprintf(f,"{\"version\":1,\"n\":%d,\"fleet\":%d,\"horizon\":%.0f,\"truck_limit\":%.0f,\"seconds\":%.9g,\"seed\":%u,\"free\":[",in->n,in->numDrones,h,tr,seconds,seed);
    for(int i=0;i<count;i++)fprintf(f,"%s%d",i?",":"",freeids[i]);
    fputs("],\"truck_times\":[",f);
    for(int i=0;i<in->n;i++){fprintf(f,"%s[",i?",":"");for(int j=0;j<in->n;j++)fprintf(f,"%s%.0f",j?",":"",in->tempoCaminhao[i][j]);fputs("]",f);}
    fputs("],\"drone_times\":[",f);
    for(int i=0;i<in->n;i++){fprintf(f,"%s[",i?",":"");for(int k=1;k<=in->numDrones;k++){if(k>1)fputs(",",f);double t=i?in->tempoDrone[i][k]:INFINITY;if(i&&clientePodeUsarDrone(in,i)&&k>=in->clientes[i].minDrones&&isfinite(t)&&t>=0)fprintf(f,"%.0f",t);else fputs("null",f);}fputs("]",f);}
    fputs("],\"route\":[",f);for(int i=0;i<base->numMissoesCaminhao;i++)fprintf(f,"%s%d",i?",":"",base->missaoCaminhao[i].n);
    fputs("],\"missions\":[",f);for(int i=0;i<base->numMissoesDrone;i++){MissaoDrone x=base->missaoDrone[i];fprintf(f,"%s[%d,%d,%.17g,%.17g]",i?",":"",x.cliente,x.qtdDrones,x.tempoInicio,x.tempoFim);}fputs("]}\n",f);
    int error=ferror(f);if(fclose(f))error=1;f=NULL;if(error)goto cleanup;
    double remaining=seconds-(relogioReparoCV()-start);if(remaining<=0){result=0;goto cleanup;}
    m->chamadasCP++;int ran=processoCPSATCV(python,script,req,res,remaining);
    if(ran<0)goto cleanup;
    if(!ran){m->limitesCP++;result=0;goto cleanup;}
    f=fopen(res,"r");if(!f)goto cleanup;char status[32];
    if(fscanf(f,"%31s",status)!=1)goto cleanup;
    if(!strcmp(status,"INFEASIBLE")){m->inviaveisCP++;result=0;goto cleanup;}
    if(!strcmp(status,"UNKNOWN")){m->limitesCP++;result=0;goto cleanup;}
    if(!strcmp(status,"UNSUPPORTED")){result=0;goto cleanup;}
    if(strcmp(status,"FEASIBLE")&&strcmp(status,"OPTIMAL"))goto cleanup;
    if(fscanf(f,"%d%d",&candidate.numMissoesCaminhao,&candidate.numMissoesDrone)!=2||candidate.numMissoesCaminhao<0||candidate.numMissoesDrone<0||candidate.numMissoesCaminhao>in->n-1||candidate.numMissoesDrone!=in->n-1-candidate.numMissoesCaminhao)goto cleanup;
    for(int i=0;i<candidate.numMissoesCaminhao;i++)if(fscanf(f,"%d",&candidate.missaoCaminhao[i].n)!=1)goto cleanup;
    for(int i=0;i<candidate.numMissoesDrone;i++){MissaoDrone *x=&candidate.missaoDrone[i];if(fscanf(f,"%d%d%lf%lf",&x->cliente,&x->qtdDrones,&x->tempoInicio,&x->tempoFim)!=4)goto cleanup;}
    char extra;if(fscanf(f," %c",&extra)==1)goto cleanup;
    /* O cronograma do CP pode ser diferente de first-fit. Avaliar sem
     * reagendar; cache incremental desativado ate a proxima mudanca nos drones. */
    candidate.cronogramaValido=false;
    candidate.cronogramaExterno=true;
    if(!calcularMakespan(in,&candidate)||candidate.makespan>h+1e-9||candidate.tempoCaminhao>tr+1e-9)goto cleanup;
    copiarCompletaReparoCV(in,&candidate,out);m->sucessosCP++;m->melhorias++;result=1;
cleanup:
    if(f)fclose(f);
    if(*req)remove(req);
    if(*res)remove(res);
    free(freeids);liberarSolucao(&candidate);m->segundosCP+=relogioReparoCV()-start;
    if(result<0)m->errosCP++;
    return result;
}
