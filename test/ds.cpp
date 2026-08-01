#include"../Nitori.h"

struct nconcat_op{static string id(){return{};}static string op(string a,const string&b){return a+=b;}};
template<class O>concept nfenwick_available=requires{typename nfenwick<string,O>;};
static_assert(!nfenwick_available<nconcat_op>);
static_assert(ncommutative_monoid<nadd<long long>,long long>);

int main(){
    mt19937 rng(9);const int n=137;nvector<long long>a(n,0);nfenwick<long long>f(n);nseg_iter<long long>st(n);nlazy_addsum<long long>lz(n);
    for(int z=0;z<10000;++z){if(rng()%3){int i=int(rng()%n),x=int(rng()%201)-100;a[i]+=x;f.add(i,x);st.set(i,a[i]);lz.apply(i,i+1,x);}else{int l=int(rng()%(n+1)),r=int(rng()%(n+1));if(l>r)swap(l,r);long long s=accumulate(a.a.begin()+l,a.a.begin()+r,0LL);assert(f.fold(l,r)==s&&st.fold(l,r)==s&&lz.fold(l,r)==s);}}
    nfenwick<int>freq(20);for(int x:{1,1,3,7,7,7,19})freq.add(x,1);assert(freq.lower(1)==1&&freq.lower(2)==1&&freq.lower(3)==3&&freq.lower(7)==19&&freq.lower(8)==npos&&freq.lower(8,99)==99);
    nvector<int>b{1,2,3,4,5};nseg_iter<int,nmax<int>>mx(b);assert(mx.fold(1,4)==4);assert(mx.maxr(0,[&](int x){return x<4;})==3);assert(mx.minl(5,[&](int x){return x<5;})==5);
    nvector<long long>c(80,0);nlazy_addsum<long long>q(c);for(int z=0;z<5000;++z){int l=int(rng()%81),r=int(rng()%81);if(l>r)swap(l,r);if(rng()&1){long long x=int(rng()%101)-50;q.apply(l,r,x);for(int i=l;i<r;++i)c[i]+=x;}else assert(q.fold(l,r)==accumulate(c.a.begin()+l,c.a.begin()+r,0LL));}
    nvector<int>spv{5,2,7,1,9,3};nsparse<int>sp(spv);for(int l=0;l<=spv.len();++l)for(int r=l;r<=spv.len();++r){int z=ninf<int>;for(int i=l;i<r;++i)z=min(z,spv[i]);assert(sp.fold(l,r)==z);}struct cat{static string id(){return{};}static string op(string a,const string&b){return a+=b;}};nqueue_agg<string,cat>qa;qa.push("ab");qa.push("c");assert(qa.fold()=="abc"&&qa.pop()=="ab");qa.push("de");assert(qa.fold()=="cde"&&qa.front()=="c");
    ndsu d(8);d.merge(0,1);d.merge(2,3);d.merge(1,3);d.merge(5,6);assert(d.same(0,2)&&d.size(3)==4&&!d.same(4,5));auto p=d.partition();assert(p.same(0,3)&&!p.same(0,4)&&p.groups().len()==4);
    ndsu_rollback r(6);int t=r.time();r.merge(0,1);r.merge(1,2);assert(r.same(0,2));int u=r.time();r.merge(2,3);assert(r.size(0)==4);r.rollback(u);assert(!r.same(0,3)&&r.same(0,2));r.rollback(t);assert(!r.same(0,1));
    npotential_dsu<long long>pd(5);assert(pd.bind(0,1,7)&&pd.bind(1,2,-3)&&pd.diff(0,2).val()==4&&pd.diff(2,0).val()==-4);assert(pd.bind(0,2,4)&&!pd.bind(0,2,5)&&!pd.diff(0,4)&&pd.diff(0,4,99)==99);
}
