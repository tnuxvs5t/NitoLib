#include"../Nitori.h"

template<class T>concept nhas_public_p=requires(T x){x.p;};
template<class T>concept nhas_public_c=requires(T x){x.c;};
template<class T>concept nhas_public_f=requires(T x){x.f;};
template<class T>concept nhas_public_a=requires(T x){x.a;};
template<class T>concept nhas_public_e=requires(T x){x.e;};

static_assert(!nhas_public_p<nperm>);
static_assert(!nhas_public_c<npart_dense>);
static_assert(!nhas_public_f<nbije<int,int>>);
static_assert(!nhas_public_a<nbije_rank<int>>);
static_assert(!nhas_public_e<nrel<int,int>>);
static_assert(same_as<decltype(declval<nbije<int,int>&>().to(0)),const int*>);
static_assert(same_as<decltype(declval<nbije_rank<int>&>().from(0)),const int*>);

template<class S>void check_set(const S&s,const set<int>&r){assert(s.len()==int(r.size()));vector<int>a;nfor(x,s)a.push_back(x);assert(equal(a.begin(),a.end(),r.begin(),r.end()));for(int x=-5;x<=55;++x){assert(s.has(x)==r.contains(x));assert(s.rank(x)==distance(r.begin(),r.lower_bound(x)));auto l=s.lower(x),u=s.upper(x);auto il=r.lower_bound(x),iu=r.upper_bound(x);assert(bool(l)==(il!=r.end()));assert(bool(u)==(iu!=r.end()));if(l)assert(*l==*il);if(u)assert(*u==*iu);}for(int i=-1;i<=s.len();++i){auto x=s.kth(i);assert(bool(x)==(0<=i&&i<s.len()));if(x){auto j=r.begin();advance(j,i);assert(*x==*j);}}}

int main(){
    nseed(1);nset<int>s;set<int>r;mt19937 g(7);for(int z=0;z<5000;++z){int x=g()%51;if(g()&1)assert(bool(s.ins(x))==r.insert(x).second);else assert(bool(s.del(x))==bool(r.erase(x)));check_set(s,r);}
    nset_splay<int>ss;set<int>sr;for(int z=0;z<5000;++z){int x=int(g()%101);if(g()&1)assert(bool(ss.ins(x))==sr.insert(x).second);else assert(bool(ss.del(x))==bool(sr.erase(x)));check_set(ss,sr);}nbag<int>bm; nset_splay<int,nless<int>,true>bms;for(int z=0;z<3000;++z){int x=int(g()%30);if(g()&1)assert(bm.ins(x)==bms.ins(x));else assert(bm.del(x)==bms.del(x));assert(bm.len()==bms.len());nrep(i,bm.len())assert(bm.kth(i).val()==bms.kth(i).val());}
    nset<int>a,b;for(int x:{1,2,4})a.ins(x);for(int x:{2,3,4})b.ins(x);nset_stl<int>ast,bst;for(int x:{1,2,4})ast.ins(x);for(int x:{2,3,4})bst.ins(x);assert((ast|bst)==nset_stl<int>({1,2,3,4})&&ast.min().val()==1&&ast.max().val()==4);
    auto u=a|b,i=a&b,d=a-b,y=a^b;vector<int>v;nfor(x,u)v.push_back(x);assert((v==vector<int>{1,2,3,4}));v.clear();nfor(x,i)v.push_back(x);assert((v==vector<int>{2,4}));v.clear();nfor(x,d)v.push_back(x);assert((v==vector<int>{1}));v.clear();nfor(x,y)v.push_back(x);assert((v==vector<int>{1,3}));
    nbag<int>m;m.ins(2,3);m.ins(1,2);m.del(2);assert(m.len()==4&&m.count(2)==2&&m.rank(2)==2&&m.kth(3).val()==2);m.delall(1);assert(!m.has(1));
    nmap<string,int>mp;assert(mp.get("x")==nullptr&&mp.get("x",8)==8);mp["x"]++;mp.set("y",4);assert(mp("x")==1&&mp("y")==4);int sum=0;nforkv(k,v,mp)sum+=int(k.size())+v;assert(sum==7);
    nmap<int,int>fm;nmap_stl<int,int>sm;unordered_map<int,int>rm;for(int z=0;z<20000;++z){int k=int(g()%200),op=int(g()%4);if(op==0){int v=int(g());bool x=fm.ins(k,v),y=sm.ins(k,v),q=rm.emplace(k,v).second;assert(x==y&&x==q);}else if(op==1){int v=int(g());fm.set(k,v);sm.set(k,v);rm[k]=v;}else if(op==2){int x=fm.del(k),y=sm.del(k),q=int(rm.erase(k));assert(x==y&&x==q);}else{auto x=fm.get(k);auto y=sm.get(k);auto q=rm.find(k);assert(bool(x)==(q!=rm.end())&&bool(y)==bool(x));if(x)assert(*x==q->second&&*y==*x);}assert(fm.len()==int(rm.size()));}nforkv(k,v,fm)assert(rm.at(k)==v);struct zh{size_t operator()(int)const{return 0;}};nmap_flat<int,int,zh>hm;unordered_map<int,int>hr;for(int z=0;z<30000;++z){int k=int(g()%1000),op=int(g()%3);if(op==0)hm[k]=hr[k]=int(g());else if(op==1)assert(hm.del(k)==int(hr.erase(k)));else{auto p=hm.get(k);auto q=hr.find(k);assert(bool(p)==(q!=hr.end())&&(!p||*p==q->second));}assert(hm.len()==int(hr.size()));}
    nbije<string,int>f;assert(f.bind("a",2)&&f.bind("b",5)&&!f.bind("a",5)&&f.to("z")==nullptr&&f.to("z",9)==9);assert(*f.to("a")==2&&*f.from(5)=="b");auto fi=~f;assert(*fi.to(2)=="a"&&*fi.from("b")==5);f.set("a",7);assert(!f.hasr(2)&&*f.to("a")==7);f.unbindr(5);assert(!f.hasl("b"));
    nvector<int>raw{30,10,30,20};auto id=ncompress(raw);assert(id.len()==3&&id.to(10)==0&&id.to(20)==1&&id.to(30)==2&&id.to(9)==npos&&*id.from(2)==30);auto rid=~id;assert(*rid.to(1)==20);
    nperm p(nvector<int>{1,2,0}),q(nvector<int>{2,0,1});auto e=p*q;for(int x=0;x<3;++x)assert(e(x)==p(q(x)));assert((p*~p)==nperm(3)&&p.pow(3)==nperm(3)&&p.sign()==1);
    nrel<string,int>rr;assert(rr.add("a",1)&&rr.add("a",2)&&!rr.add("a",1)&&rr.has("a",2));assert((rr.image("a")==nvector<int>{1,2})&&(rr.preimage(1)==nvector<string>{"a"}));nrel<string,int>rr2;rr2.add("a",2);rr2.add("b",3);auto ri=rr&rr2;assert(ri.len()==1&&ri.has("a",2)&&!ri.has("a",1)&&(rr|rr2).len()==3);nrel<int,int>ra,rb;ra.add(1,2);rb.add(1,2);assert((ra&rb).has(1,2));nfunc<string,int>fn;assert(fn.bind("x",4)&&!fn.bind("x",5)&&fn.to("z",8)==8);fn.set("x",7);assert(fn("x")==7&&fn.unbind("x")&&!fn.has("x"));
}
