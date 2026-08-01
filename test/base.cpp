#include"../Nitori.h"

int main(){
    static_assert(ninf<int> ==numeric_limits<int>::max()/4);
    nmaybe<int>a;a=nmaybe<int>(7);assert(a&&a.val()==7&&a.val(3)==7);a.reset();assert(!a&&a.val(3)==3);
    int x=5;assert(nchmin(x,3)&&x==3);assert(!nchmin(x,4));assert(nchmax(x,9)&&x==9);
    vector<int>v;
    nfor(i,nrange(5))v.push_back(i);
    assert((v==vector<int>{0,1,2,3,4}));v.clear();
    nfor(i,nrange(5,0,-2))v.push_back(i);
    assert((v==vector<int>{5,3,1}));
    int z=0,eval=0;nrep(i,++eval)z+=i;assert(eval==1&&z==0);
    int nested=0;nrep(i,3)nrep(j,4)nested+=i+j;assert(nested==30);
    int cut=0;nfor(i,nrange(10)){if(i==4)break;++cut;}assert(cut==4);
    int skip=0;nfor(i,nrange(6)){if(i&1)continue;skip+=i;}assert(skip==6);
    int weighted=0;nfori(i,x,nrange(2,8,2))weighted+=i*x;assert(weighted==16);
    int raw[]={4,1,9};nspan<int>s(raw,3);assert(s.len()==3&&s[1]==1&&s.get(4)==nullptr&&s.get(4,8)==8);
    v.clear();nfor(x,s)v.push_back(x);assert((v==vector<int>{4,1,9}));s.sub(1)[0]=7;assert(raw[1]==7);
    struct X{int x=0;X()=default;X(int x):x(x){}};npool<X>p;int i=p.make(4),j=p.make(9);assert(p.len()==2&&p[i].x==4&&p[j].x==9);p.del(i);int k=p.make(6);assert(k==i&&p[k].x==6&&p.len()==2);
    assert(npow(3LL,5)==243);assert(npow(2,10,nmul<int>{})==1024);assert(npow(7,0)==1);
    nseed(123);auto r1=nrng_global();nseed(123);auto r2=nrng_global();assert(r1==r2);
    auto hi=nrange(LLONG_MAX-1,LLONG_MAX,2LL);assert(hi.len()==1&&hi.enumerate().val()==LLONG_MAX-1);auto lo=nrange(LLONG_MIN,LLONG_MIN+1,-2LL);assert(lo.len()==0);auto rv=nreverse(nrange(LLONG_MAX,LLONG_MIN,LLONG_MIN));assert(rv.len()==2&&rv.enumerate().val()==-1);
}
