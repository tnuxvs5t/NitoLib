#include"../Nitori.h"

int main(){
    string s="aabcaabxaaaz";auto z=nzfunc(s);nrep(i,int(s.size())){int k=0;while(i+k<int(s.size())&&s[k]==s[i+k])++k;assert(z[i]==k);}auto p=nprefix(string("ababcabab"));assert((p==nvector<int>{0,0,1,2,0,1,2,3,4}));assert((nkmp(string("ababa"),string("aba"))==nvector<int>{0,2}));assert((nkmp(string("abc"),string())==nvector<int>{0,1,2,3}));
    auto m=nmanacher(string("abacaba"));assert(m.pal(0,7)&&m.pal(1,6)&&!m.pal(0,6)&&m.pal(3,3));
    mt19937 g(11);for(int tc=0;tc<500;++tc){int n=int(g()%30);string x(n,'a');for(char&c:x)c=char('a'+g()%4);auto sa=nsuffix_array(x),lcp=nlcp_array(x,sa);vector<int>ref(n);iota(ref.begin(),ref.end(),0);sort(ref.begin(),ref.end(),[&](int i,int j){return x.substr(i)<x.substr(j);});assert(sa.a==ref);for(int i=1;i<n;++i){int h=0;while(sa[i]+h<n&&sa[i-1]+h<n&&x[sa[i]+h]==x[sa[i-1]+h])++h;assert(lcp[i]==h);}auto q=nmanacher(x);for(int l=0;l<=n;++l)for(int r=l;r<=n;++r){bool ok=true;for(int i=0;i<(r-l)/2;++i)ok&=x[l+i]==x[r-1-i];assert(q.pal(l,r)==ok);}}
    nac<>ac;ac.add(string("he"));ac.add(string("she"));ac.add(string("his"));ac.add(string("hers"));ac.build();auto c=ac.count(string("ahishers"));assert((c==nvector<long long>{1,1,1,1}));auto ms=ac.matches(string("ahishers"));assert(ms.len()==4&&ac.step(0,'?')==0);
    for(int tc=0;tc<200;++tc){nac<3>q;vector<string>a;int k=1+int(g()%12);nrep(i,k){int n=1+int(g()%5);string w(n,'a');for(char&c:w)c=char('a'+g()%3);a.push_back(w);q.add(w);}q.build();int n=int(g()%40);string x(n,'a');for(char&c:x)c=char('a'+g()%3);auto got=q.count(x);nrep(i,k){long long want=0;for(int j=0;j+int(a[i].size())<=n;++j)want+=x.compare(j,a[i].size(),a[i])==0;assert(got[i]==want);}}
}
