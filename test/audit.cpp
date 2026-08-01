#include"../Nitori.h"

struct npool_life{
    static inline int live=0;
    int x=0;
    npool_life(){++live;}
    explicit npool_life(int x):x(x){++live;}
    npool_life(const npool_life&x):x(x.x){++live;}
    npool_life&operator=(const npool_life&)=default;
    ~npool_life(){--live;}
};

int main(){
    {
        npool<npool_life>p;
        int a=p.make(1),b=p.make(2);
        assert(npool_life::live==2);
        p.del(a);assert(npool_life::live==1&&p.get(a)==nullptr);
        int c=p.make(3);assert(c==a&&p[c].x==3&&npool_life::live==2);
        p.clear();assert(npool_life::live==0);
        (void)b;
    }
    nmap_flat<int,int>m;m.reserve(10000);nrep(i,10000)m.ins(i,i*i);nrep(i,10000)assert(m.get(i)&&*m.get(i)==i*i);nrep(i,10000)assert(m.del(i));assert(m.empty());
    npath_result<int>r{nvector<int>{7},nvector<int>{0},7};assert(r.reach(0)&&r.dist(0,99)==7);
    npath_result<int>u{nvector<int>{7},nvector<int>{npos},7};assert(!u.reach(0)&&u.dist(0,99)==99);
    nprob<double>bad{1,-1};assert(!bad.nonnegative()&&!bad.normalized());assert(bad.draw(nrng_global,42)==42);
    nvector<int>e;nzeta_subset(e);nzeta_superset(e,true);assert(e.empty());
    auto eg=nextgcd(LLONG_MIN,0);assert(eg.g==(__int128(1)<<63)&&__int128(LLONG_MIN)*eg.x==eg.g);
    auto eh=nextgcd(0,LLONG_MIN);assert(eh.g==(__int128(1)<<63)&&__int128(LLONG_MIN)*eh.y==eh.g);
    nvector<int>a{1,2,3,4,5};nseg_iter<int>s(a);
    for(int l=0;l<=a.len();++l)for(int t=0;t<=16;++t){int want=l,sum=0;while(want<a.len()&&sum+a[want]<=t)sum+=a[want++];assert(s.maxr(l,[&](int x){return x<=t;})==want);}
    for(int r=0;r<=a.len();++r)for(int t=0;t<=16;++t){int want=r,sum=0;while(want>0&&sum+a[want-1]<=t)sum+=a[--want];assert(s.minl(r,[&](int x){return x<=t;})==want);}
}
