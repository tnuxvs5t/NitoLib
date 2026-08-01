#include"../Nitori.h"
using P=npoint<long long>;

int main(){
    static_assert(is_same_v<decltype(ncross(P{},P{})),__int128_t>);P a{0,0},b{4,0},c{2,2},d{2,-2};assert(ncross(a,b,c)==8&&ndot(P{2,3},P{4,-1})==5&&nsegment_intersect(a,b,c,d));assert(nsegment_intersect(a,b,P{4,0},P{8,0})&&!nsegment_intersect(a,b,P{5,0},P{8,0}));
    nline2<long long>l1{{0,0},{1,1}},l2{{0,2},{1,-1}};auto q=nline_intersect(l1,l2);assert(q&&abs(q->x-1)<1e-12&&abs(q->y-1)<1e-12);assert(!nline_intersect(l1,{{1,0},{1,1}}));
    nvector<P>x{{0,0},{2,0},{2,2},{0,2},{1,1},{2,0}};auto h=nconvex_hull(x);assert((h==nvector<P>{{0,0},{2,0},{2,2},{0,2}})&&npolygon_area2(h)==8&&nconvex_diameter2(h)==8);assert(npoint_in_poly(h,P{1,1})==1&&npoint_in_poly(h,P{2,1})==0&&npoint_in_poly(h,P{3,1})==-1);nvector<P>col{{2,0},{0,0},{1,0}};assert((nconvex_hull(col,true)==nvector<P>{{0,0},{1,0},{2,0}}));
    mt19937 g(12);for(int tc=0;tc<500;++tc){nvector<P>v;int n=1+int(g()%50);nrep(i,n)v.push(P{int(g()%41)-20,int(g()%41)-20});auto ch=nconvex_hull(v);if(ch.len()>2)nrep(i,ch.len())assert(norient(ch[i],ch[(i+1)%ch.len()],ch[(i+2)%ch.len()])>0);nfor(p,v)assert(npoint_in_poly(ch,p)>=0);__int128 want=0;nfor(u,ch)nfor(vv,ch)nchmax(want,ndist2(u,vv));assert(nconvex_diameter2(ch)==want);}
}
