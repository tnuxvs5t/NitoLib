#include"../Nitori.h"
#include<sys/resource.h>

using clk=chrono::steady_clock;
template<class F>long long bench(const char*name,F f){auto t=clk::now();uint64_t z=f();auto u=chrono::duration_cast<chrono::microseconds>(clk::now()-t).count();cout<<name<<","<<u<<","<<z<<endl;return u;}
static uint64_t rnd(uint64_t&x){x^=x<<7;x^=x>>9;return x;}

int main(){
    ios::sync_with_stdio(false);cin.tie(nullptr);nseed(1);uint64_t s=0x123456789abcdef0ULL;
    const int N=20000;vector<int>k(N);nrep(i,N)k[i]=i;for(int i=N;i--;)swap(k[i],k[rnd(s)%(i+1)]);
    bench("map_flat",[&]{nmap_flat<int,int>m(N);for(int x:k)m.ins(x,x);uint64_t z=0;for(int x:k)if(auto p=m.get(x))z+=*p;for(int i=0;i<N;i+=3)m.del(k[i]);return z+uint64_t(m.len());});
    bench("map_stl",[&]{nmap_stl<int,int>m(N);for(int x:k)m.ins(x,x);uint64_t z=0;for(int x:k)if(auto p=m.get(x))z+=*p;for(int i=0;i<N;i+=3)m.del(k[i]);return z+uint64_t(m.len());});
    bench("set_fhq",[&]{nset_fhq<int>s;for(int x:k)s.ins(x);uint64_t z=0;for(int x:k)z+=s.rank(x);for(int i=0;i<N;i+=3)s.del(k[i]);return z+uint64_t(s.len());});
    bench("set_splay",[&]{nset_splay<int>s;for(int x:k)s.ins(x);uint64_t z=0;for(int x:k)z+=s.rank(x);for(int i=0;i<N;i+=3)s.del(k[i]);return z+uint64_t(s.len());});
    bench("set_stl",[&]{nset_stl<int>s;for(int x:k)s.ins(x);uint64_t z=0;for(int x:k)z+=s.rank(x);for(int i=0;i<N;i+=3)s.del(k[i]);return z+uint64_t(s.len());});
    const int Q=50000,M=1<<17;
    bench("fenwick",[&]{nfenwick<int>f(M);uint64_t z=0;for(int i=0;i<Q;++i){int x=int(rnd(s)%M);f.add(x,1);z+=f.prefix(x+1);}return z;});
    bench("segment",[&]{nseg_iter<int>f(M);uint64_t z=0;for(int i=0;i<Q;++i){int x=int(rnd(s)%M);f.set(x,f.get(x)+1);z+=f.fold(0,x+1);}return z;});
    using mint=nmod<998244353>;
    nvector<mint>a(1<<10),b(1<<10);nrep(i,a.len())a[i]=int(rnd(s)%1000),b[i]=int(rnd(s)%1000);
    bench("conv_naive_1024",[&]{auto c=nconv_naive(a,b);return uint64_t(c.len())+uint64_t(c[17]);});
    nvector<mint>aa(1<<13),bb(1<<13);nrep(i,aa.len())aa[i]=int(rnd(s)%1000),bb[i]=int(rnd(s)%1000);
    bench("conv_ntt_8192",[&]{auto c=nconv_ntt(aa,bb);return uint64_t(c.len())+uint64_t(c[17]);});
    const int V=20000,E=50000;ngraph<int>g(V,E);for(int i=0;i<E;++i){int u=int(rnd(s)%V),v=int(rnd(s)%V);g.add(u,v,int(rnd(s)%100)+1);}
    bench("dijkstra",[&]{auto d=ndijkstra(g,0,1000000000);uint64_t z=0;nrep(i,V)if(d.reach(i))z+=uint64_t(d[i]);return z;});
    const int F=120; nflow<int>fl(F,1500);for(int i=0;i<1500;++i){int u=int(rnd(s)%(F-1)),v=u+1+int(rnd(s)%(F-u-1));fl.add(u,v,int(rnd(s)%100)+1);}bench("dinic",[&]{return uint64_t(fl(0,F-1));});
    rusage ru{};getrusage(RUSAGE_SELF,&ru);cerr<<"maxrss_kib="<<ru.ru_maxrss<<'\n';
}
