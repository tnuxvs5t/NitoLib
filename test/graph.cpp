#include"../Nitori.h"

struct test_graph{
    nvector<nvector<nedge<int>>>a;
    int m=0;
    explicit test_graph(int n):a(n){}
    int len()const{return a.len();}
    const auto&operator[](int u)const{return a[u];}
    void add(int u,int v,int w=1){a[u].push(nedge<int>{u,v,m++,w});}
};
struct edge_data{int cost,bit;};
template<class G>void check_graph_protocol(const G&g){int vn=0,en=0;nfor(u,nvertices(g))assert(u==vn++);nfor(e,narcs(g)){assert(0<=e.from&&e.from<g.len()&&0<=e.to&&e.to<g.len());++en;}assert(vn==g.len()&&en==narcs(g).len());}

int main(){
    ngraph<>empty;check_graph_protocol(empty);assert(empty.vertices().len()==0&&empty.arcs().len()==0&&ntopo(empty)&&nscc(empty).classes()==0);ngraph_csr empty_csr(empty);check_graph_protocol(empty_csr);assert(empty_csr.len()==0&&empty_csr.edges()==0&&empty_csr.arcs().len()==0);auto empty_view=ngraph_where(empty,[](auto){return true;});check_graph_protocol(empty_view);assert(empty_view.edges()==0&&empty_view.arcs().len()==0);
    ngraph<int>g(7);g.add(0,1,2);g.add(0,2,5);g.add(1,2,1);g.add(1,3,4);g.add(2,3,1);g.add(3,4,3);g.add(5,6,1);
    auto b=nbfs(g,0);assert(b[4]==3&&!b.reach(6)&&b.dist(6,99)==99&&(b.path(4)==nvector<int>{0,2,3,4}));auto d=ndijkstra(g,0);assert(d[4]==7&&!d.reach(6)&&(d.path(4)==nvector<int>{0,1,2,3,4}));
    auto t=ntopo(g);assert(t&&t->len()==7);g.add(4,1);assert(!ntopo(g));
    ngraph<>s(8);s.add(0,1);s.add(1,2);s.add(2,0);s.add(2,3);s.add(3,4);s.add(4,3);s.add(4,5);s.add(5,6);s.add(6,7);s.add(7,6);auto p=nscc_tarjan(s);assert(p.same(0,2)&&p.same(3,4)&&p.same(6,7)&&!p.same(2,3)&&p.classes()==4);
    nflow<long long>f(6);f.add(0,1,10);f.add(0,2,10);f.add(1,2,2);f.add(1,3,4);f.add(1,4,8);f.add(2,4,9);f.add(4,3,6);f.add(3,5,10);f.add(4,5,10);assert(f(0,5)==19);auto c=f.cut(0);assert(c[0]&&!c[5]);f.reset();assert(f(0,5,7)==7);
    mt19937 rng(1);for(int z=0;z<200;++z){int n=2+int(rng()%18);ngraph<int>x(n);vector<vector<int>>a(n,vector<int>(n,1000000));nrep(i,n)a[i][i]=0;for(int e=0;e<n*n/3;++e){int u=int(rng()%n),v=int(rng()%n),w=int(rng()%20);x.add(u,v,w);a[u][v]=min(a[u][v],w);}nrep(k,n)nrep(i,n)nrep(j,n)nchmin(a[i][j],a[i][k]+a[k][j]);ngraph_csr y(x);nrep(s,n){auto q=ndijkstra(x,s,1000000),r=ndijkstra(y,s,1000000);nrep(v,n)assert(q[v]==a[s][v]&&r[v]==a[s][v]);}}
    for(int z=0;z<300;++z){int n=1+int(rng()%12);ngraph<>x(n);vector<vector<char>>r(n,vector<char>(n));nrep(i,n)r[i][i]=1;nrep(i,n)nrep(j,n)if(rng()%4==0)x.add(i,j),r[i][j]=1;nrep(k,n)nrep(i,n)nrep(j,n)r[i][j]|=r[i][k]&r[k][j];auto a=nscc(x),b=nscc_tarjan(x);nrep(i,n)nrep(j,n)assert(a.same(i,j)==bool(r[i][j]&&r[j][i])&&a.same(i,j)==b.same(i,j));}
    for(int z=0;z<300;++z){int n=2+int(rng()%7);nflow<int>x(n);vector<tuple<int,int,int>>e;nrep(u,n)nrep(v,n)if(u!=v&&rng()%4==0){int c=int(rng()%8);if(c)x.add(u,v,c),e.push_back({u,v,c});}int got=x(0,n-1),want=numeric_limits<int>::max();for(int m=0;m<(1<<n);++m)if((m&1)&&!(m>>(n-1)&1)){int c=0;for(auto[u,v,w]:e)if((m>>u&1)&&!(m>>v&1))c+=w;want=min(want,c);}assert(got==want);}
    ngraph<>dag(4);dag.add(0,1);dag.add(0,2);dag.add(1,3);dag.add(2,3);auto sg=nsg_dag(dag);assert(sg&&(*sg)[0]==0&&(*sg)[1]==(*sg)[2]&&(*sg)[3]==0);dag.add(3,0);assert(!nsg_dag(dag));
    ngraph<int>op(4);int e01=op.add(0,1,2),e12=op.add(1,2,3);op.add(0,3,9);assert(op.degree(0)==2&&op.find(0,1)==e01&&op.find(2,0)==npos&&op.find(2,0,77)==77&&op.has(1,2)&&!op.has(2,1));assert(op.weight(e12)&&*op.weight(e12)==3&&!op.weight(99)&&op.weight(99,8)==8&&op.set(e12,4)&&!op.set(99,1));int vc=0,ec=0;nfor(u,op.vertices())vc+=u;nfor(e,op.arcs()){assert(e.from>=0&&e.from<op.len()&&e.to>=0&&e.to<op.len());++ec;e.w+=1;}check_graph_protocol(op);assert(vc==6&&ec==op.edges()&&op.arcs().len()==op.edges()&&*op.weight(e01)==3&&*op.weight(e12)==5&&ndijkstra(op,0)[2]==8);const auto&cop=op;int sum=0;nfor(e,cop.arcs())sum+=e.w;assert(sum==18);ngraph_csr oc(op);check_graph_protocol(oc);int oe=oc.find(0,1);assert(oc.len()==op.len()&&oc.edges()==op.edges()&&oc.degree(0)==2&&oc.has(0,1)&&ndijkstra(oc,0)[2]==8&&oc.set(oe,4)&&*oc.weight(oe)==4&&oc.set(oe,3));auto otr=oc.reverse();check_graph_protocol(otr);assert(otr.has(1,0)&&otr.has(2,1)&&nbfs(otr,2).reach(0));
    ngraph<edge_data>pg(4);pg.add(0,1,{2,1});pg.add(1,2,{3,0});pg.add(0,2,{10,1});pg.add(2,3,{1,1});auto pd=ndijkstra(pg,0,[](auto e){return e.w.cost;});assert(pd[3]==6);auto active=ngraph_where(pg,[](auto e){return e.w.bit;});auto ad=ndijkstra(active,0,[](auto e){return e.w.cost;});assert(ad[3]==11&&nbfs(active,0).reach(1));auto bits=n01bfs(pg,0,[](auto e){return e.w.bit;});assert(bits[2]==1&&bits[3]==2);
    test_graph custom(4);custom.add(0,1,2);custom.add(1,0,2);custom.add(1,2,3);custom.add(2,1,3);custom.add(2,3,4);custom.add(3,2,4);check_graph_protocol(custom);auto cd=ndijkstra(custom,0);assert(cd[3]==9);auto cp=nscc(custom);assert(cp.classes()==1);nlca_binary cl(custom);assert(cl.lca(1,3)==1&&cl.dist(0,3,-1)==9);nhld ch(custom);assert(ch.lca(1,3)==1);int ce=0;nfor(e,narcs(custom))ce+=e.w;assert(ce==18&&narcs(custom).len()==6);
}
