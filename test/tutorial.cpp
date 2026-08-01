#include"../Nitori.h"

struct tutorial_gcd{
    static long long id(){return 0;}
    static long long op(long long a,const long long&b){return ngcd(a,b);}
};

int main(){
    nvector<long long>a{3,1,2};auto id=ncompress(a);assert(id.to(1)==0&&id.to(3)==2&&id.from(1,-1)==2);
    nfenwick<long long>f(id.len());long long inv=0;nrep(i,a.len()){int x=id.to(a[i]);inv+=i-f.prefix(x+1);f.add(x,1);}assert(inv==2);
    ngraph<long long>g(4,4);g.add(0,1,5);g.add(0,2,1);g.add(2,1,1);g.add(1,3,2);auto d=ndijkstra(g,0);assert(d.dist(3,-1)==4&&(d.path(3)==nvector<int>{0,2,1,3}));
    nset<int>s;s.ins(5);s.ins(2);s.ins(9);assert(s.rank(7)==2&&s.kth(1,-1)==5&&s.lower(6,-1)==9);
    nbije<string,int>b;assert(b.bind("alice",0)&&b.bind("bob",1)&&*b.to("alice")==0&&*b.from(1)=="bob");auto c=~b;assert(*c.to(0)=="alice");
    nvector<long long>v{12,18,30};nseg_iter<long long,tutorial_gcd>st(v);assert(st.fold(0,3)==6&&st.fold(1,3)==6);
}
