#include"../Nitori.h"

int main(){
    nvector<int>a{1,2,3};a.push(4);a+=5;assert(a.len()==5&&a.pop()==5&&a.get(9)==nullptr&&a.get(9,7)==7);
    vector<int>v;nfor(x,a)v.push_back(x);assert((v==vector<int>{1,2,3,4}));v.clear();nfor(x,nreverse(a))v.push_back(x);assert((v==vector<int>{4,3,2,1}));
    a.del(1);assert((a.a==vector<int>{1,3,4}));int z=a.swapdel(0);assert(z==1&&a.len()==2);
    static_assert(narray<int,3>::rank()==3);
    narray<int,3>ar(2,3,4);assert(ar.len()==24&&!ar.empty()&&ar.dim(0)==2&&ar.dim(2)==4&&ar.dim(3)==npos&&ar.dim(3,9)==9);
    auto sh=ar.shape();assert(sh.len()==3&&sh[0]==2&&sh[1]==3&&sh[2]==4);
    nrep(i,2){nrep(j,3){nrep(k,4)ar(i,j,k)=ar.pos(i,j,k);}}
    nrep(p,ar.len())assert(ar[p]==p);
    assert(ar.pos({1,2,3})==23&&ar.pos({-1,0,0})==npos&&ar.pos({2,0,0})==npos&&ar.pos({2,0,0},17)==17);
    assert(ar.get(24)==nullptr&&ar.get(24,-1)==-1&&ar.get({2,0,0})==nullptr&&ar.get({2,0,0},-2)==-2);
    const auto&car=ar;assert(car(1,2,3)==23&&*car.get({1,2,3})==23&&car.get({1,2,3},-1)==23);
    nreverse_inplace(ar);assert(ar[0]==23&&ar[23]==0);nreverse_inplace(ar);assert(ar[0]==0&&ar[23]==23);
    ar.fill(5);nfori(i,x,ar)assert(i<ar.len()&&x==5);
    narray<int,2>af({2,3},7);assert(af.len()==6&&nfold(af)==42);
    narray<int,2>as(2,3);nrep(i,as.len())as[i]=as.len()-1-i;nsort(as);nrep(i,as.len())assert(as[i]==i);
    narray<int,3>az({2,0,4},9);assert(az.empty()&&az.len()==0&&az.dim(0)==2&&az.dim(1)==0&&az.dim(2)==4&&az.pos({0,0,0})==npos);
    narray<int,2>ae;assert(ae.empty()&&ae.dim(0)==0&&ae.dim(1)==0);
    narray<int,1>ao(3);ao(2)=8;assert(ao[2]==8);
    narray<unique_ptr<int>,2>am(1,2);am(0,1)=make_unique<int>(6);assert(*am(0,1)==6);
#ifdef NDEBUG
    narray<int,2>an({-1,2}),ax({INT_MAX,2});if(!an.empty()||an.dim(0)!=0||!ax.empty()||ax.dim(0)!=0)return 1;
#endif
    ndeque<int>d;for(int i=0;i<1000;++i)(i&1?d.pushl(i):d.pushr(i));assert(d.len()==1000);
    deque<int>q;for(int i=0;i<1000;++i)(i&1?q.push_front(i):q.push_back(i));nfori(i,x,d)assert(x==q[i]);
    for(int i=0;i<500;++i){assert(d.popl()==q.front());q.pop_front();assert(d.popr()==q.back());q.pop_back();}assert(d.empty()&&d.popl(8)==8);
    v.clear();nfor(x,nreverse(nrange(2,9,2)))v.push_back(x);assert((v==vector<int>{8,6,4,2}));
    int s=0;nfor(p,nzip(nrange(4),nrange(10,20))){auto[x,y]=p;s+=x*y;}assert(s==74);
    vector<pair<int,int>>ps;nfor(p,nproduct(nrange(2),nrange(3)))ps.push_back(p);assert((ps==vector<pair<int,int>>{{0,0},{0,1},{0,2},{1,0},{1,1},{1,2}}));
    nvector<int>b{4,1,3,1,2};nsort(b);assert((b==nvector<int>{1,1,2,3,4})&&nlower(b,1)==0&&nupper(b,1)==2&&nfind_sorted(b,3)==3&&nfind_sorted(b,9)==npos&&nfold(b)==11);nunique(b);assert((b==nvector<int>{1,2,3,4}));nreverse_inplace(b,1,4);assert((b==nvector<int>{1,4,3,2}));
    nheap<int>h;priority_queue<int,vector<int>,greater<int>>rh;mt19937 g(9);for(int i=0;i<5000;++i)if(rh.empty()||g()%3){int x=int(g()%1000);h.push(x);rh.push(x);}else{assert(h.top()==rh.top()&&h.pop()==rh.top());rh.pop();}while(!rh.empty()){assert(h.pop()==rh.top());rh.pop();}assert(h.pop(7)==7);
    struct item{int k,id;};struct cmp{bool operator()(const item&a,const item&b)const{return a.k<b.k;}};nheap_binary<item,cmp>hh;auto&ref=hh.push(9,1);assert(ref.id==1);auto&ref2=hh.push(1,2);assert(ref2.id==2&&hh.top().id==2);
}
