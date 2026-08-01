#include"../Nitori.h"

int main(){
    FILE*f=tmpfile();assert(f);fputs("-9223372036854775808 18446744073709551615 word 3.5",f);rewind(f);ninput in(f);long long a;unsigned long long b;string s;double x;assert(in.read(a,b,s,x)&&a==LLONG_MIN&&b==ULLONG_MAX&&s=="word"&&abs(x-3.5)<1e-12);assert(!in.next<int>()&&in.next<int>(17)==17);fclose(f);
    f=tmpfile();assert(f);{noutput out(f);out<<LLONG_MIN;out.space();out<<ULLONG_MAX;out.space();out<<__int128(-1234567890123456789LL);out.space();out<<string("ok");out.line();out.flush();rewind(f);char z[256]{};fread(z,1,sizeof z-1,f);assert(string(z)=="-9223372036854775808 18446744073709551615 -1234567890123456789 ok\n");}fclose(f);
}
