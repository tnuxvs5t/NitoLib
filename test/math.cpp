#include"../Nitori.h"
using mint=nmod<998244353>;

int main(){
    assert(nfloor_div(-7,3)==-3&&nceil_div(-7,3)==-2&&ngcd(84,30)==6);assert(nabs(LLONG_MIN)==(1ULL<<63)&&ngcd(LLONG_MIN,0LL)==(1ULL<<63)&&nfloor_div(LLONG_MIN,1LL)==LLONG_MIN);auto e=nextgcd(84,30);assert(e.g==6&&84*e.x+30*e.y==6);auto ee=nextgcd(LLONG_MAX,LLONG_MAX-2);assert(__int128(LLONG_MAX)*ee.x+__int128(LLONG_MAX-2)*ee.y==ee.g);
    nfrac<>fr(2,-4),fg(5,6);assert(fr==nfrac<>(-1,2)&&fr+fg==nfrac<>(1,3)&&fr*fg==nfrac<>(-5,12)&&fg/fr==nfrac<>(-5,3)&&fr.floor()==-1&&fr.ceil()==0);assert(!fr.trydiv(nfrac<>(0)));
    auto c=ncrt({2,6},{5,9});assert(c&&c->m==18&&c->a==14);assert(!ncrt({1,2},{0,2}));ncongruence ce(LLONG_MIN,LLONG_MAX);assert(ce.has(ce.a)&&!ce.at(2)&&ce.at(0).val()==ce.a);assert(!ncrt({0,LLONG_MAX},{0,LLONG_MAX-1}));
    mint a=-3,b=5;assert(uint64_t(a+b)==2&&uint64_t(a*b)==998244338);assert(uint64_t(b/b)==1&&uint64_t(npow(b,5))==3125);assert(!mint(0).tryinv()&&mint(0).inv(mint(7))==mint(7));
    ndmod<0>::setmod(17);ndmod<1>::setmod(12);ndmod<0>da=20;ndmod<1>db=20;assert(uint64_t(da)==3&&uint64_t(db)==8&&uint64_t(da/ndmod<0>(3))==1&&!db.tryinv());
    for(uint64_t p:{2ULL,3ULL,5ULL,97ULL,1000000007ULL,2305843009213693951ULL})assert(nisprime(p));for(uint64_t x:{0ULL,1ULL,4ULL,91ULL,1000000009ULL*1000000007ULL})assert(!nisprime(x));nseed(4);uint64_t x=1000000007ULL*1000000009ULL;auto fs=nfactor(x);assert((fs==vector<uint64_t>{1000000007ULL,1000000009ULL}));
    nprime_table pt(100);assert(pt.isprime(97)&&pt.phi[36]==12&&pt.mu[30]==-1);assert((pt.factor(72)==vector<pair<int,int>>{{2,3},{3,2}}));assert((pt.divisors(12)==vector<int>{1,2,3,4,6,12}));
    ncomb<mint>co(20);assert(co.C(5,2)==mint(10)&&co.P(5,2)==mint(20)&&co.H(3,2)==mint(6)&&co.C(30,2,mint(9))==mint(9));
    nvector<int>z{1,2,3,4,5,6,7,8},o=z;nzeta_subset(z);nzeta_subset(z,true);assert(z==o);
    nmat<mint>A(2,2);A(0,0)=1;A(0,1)=1;A(1,0)=1;auto F=A.pow(10);assert(F(0,1)==mint(55));nxorbasis<>xb;assert(xb.ins(3)&&xb.ins(5)&&!xb.ins(6)&&xb.has(6)&&xb.max()==6);
    nmat<mint>G(2,3);G(0,0)=1;G(0,1)=1;G(0,2)=1;G(1,0)=2;G(1,1)=3;G(1,2)=1;nvector<mint>gb{6,11};auto gs=ngauss(G,gb);assert(gs.consistent&&gs.rank==2&&gs.basis.len()==1);nrep(i,2){mint z{};nrep(j,3)z+=G(i,j)*gs.one[j];assert(z==gb[i]);}nmat<mint>D(3,3);D(0,0)=2;D(0,1)=1;D(1,0)=1;D(1,1)=2;D(2,2)=3;auto Di=ninverse(D);assert(Di&&D*Di.val()==nmat<mint>::eye(3)&&ndet(D)==mint(9));nmat<mint>Sg(2,2);Sg(0,0)=Sg(1,0)=1;assert(!ninverse(Sg));
    nvector<mint>fib(20);fib[1]=1;for(int i=2;i<20;++i)fib[i]=fib[i-1]+fib[i-2];auto rc=nberlekamp(fib);assert((rc==nvector<mint>{1,1})&&nrec_nth(nvector<mint>{0,1},rc,50).val()==mint(607336789));assert(!nrec_nth(nvector<mint>{1},nvector<mint>{},5));
    mt19937 rng(3);for(int n=0;n<100;++n)for(int m=0;m<100;++m){nvector<mint>u(n),v(m);nfor(x,u)x=int(rng()%100);nfor(x,v)x=int(rng()%100);assert(nconv_auto(u,v)==nconv_naive(u,v));}
    npoly<mint>P{1,2,3},Q{3,4};auto R=P*Q;assert((R==npoly<mint>{3,10,17,12}));assert(P(mint(2))==mint(17));assert((P.deriv()==npoly<mint>{2,6}));assert((P.deriv().integral()==npoly<mint>{0,2,3}));
    for(int n=1;n<=128;++n){nvector<mint>v(n);v[0]=1;for(int i=1;i<n;++i)v[i]=int(rng()%100);npoly<mint>f(v);auto iv=f.inv(n),one=(f*iv).cut(n);assert(one[0]==mint(1));for(int i=1;i<n;++i)assert(one[i]==mint(0));auto lg=f.log(n),ex=lg.exp(n);nrep(i,n)assert(ex[i]==f[i]);}npoly<mint>X{0,1};auto ex=X.exp(10);mint fac=1;nrep(i,10){if(i)fac*=i;assert(ex[i]==mint(1)/fac);}
}
