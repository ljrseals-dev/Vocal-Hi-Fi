#include "VocalProcessor.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <limits>
int main(){int fail=0;auto check=[&](bool ok,const char*n){std::printf("%s %s\n",ok?"PASS":"FAIL",n);if(!ok)++fail;};
 vh::VocalProcessor p;p.prepare(48000,2);std::vector<float> l(48000),r(48000);float* ch[2]={l.data(),r.data()};
 for(int i=0;i<48000;++i){l[i]=r[i]=.1f*std::sin(2*3.14159265*220*i/48000);}
 p.process(ch,2,48000);float peak=0;bool finite=true;for(int i=0;i<48000;++i){peak=std::max(peak,std::max(std::fabs(l[i]),std::fabs(r[i])));finite&=std::isfinite(l[i])&&std::isfinite(r[i]);}check(finite&&peak<=1.001f,"finite/headroom");
 p.reset();std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);p.process(ch,2,48000);peak=0;for(float x:l)peak=std::max(peak,std::fabs(x));check(peak<1e-6f,"silence");
 p.reset();std::fill(l.begin(),l.end(),.1f);l[24000]=std::numeric_limits<float>::quiet_NaN();p.process(ch,2,48000);finite=true;for(float x:l)finite&=std::isfinite(x);check(finite,"NaN sanitization");
 p.reset();std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);p.process(ch,2,48000);peak=0;for(float x:l)peak=std::max(peak,std::fabs(x));check(peak<1e-6f,"ADT zero stability");
 check(vh::VocalProcessor::latencySamples()==0,"zero latency");
 return fail?1:0;}
