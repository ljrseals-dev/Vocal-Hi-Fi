#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace vh {
class ADT {
public:
 struct Params { float delayMs=22, depthMs=1.5f, rateHz=.35f, wet=.35f, spread=.85f; };
 void prepare(double sr,int channels) {
  sr_=std::max(8000.0,sr); ch_=std::max(1,std::min(2,channels));
  const size_t n=static_cast<size_t>(std::ceil(sr_*.070))+8;
  for(auto &v: ring_) v.assign(n,0.0f);
  write_=0; phase_=0; rng_=0x1234567u; drift_=0; hpX_.fill(0); hpY_.fill(0); lpY_.fill(0);
 }
 void reset(){ for(auto&v:ring_) std::fill(v.begin(),v.end(),0.0f); write_=0; phase_=0; drift_=0; hpX_.fill(0); hpY_.fill(0); lpY_.fill(0); }
 void setParams(const Params&p){ p_=p; p_.delayMs=clamp(p_.delayMs,2,60);p_.depthMs=clamp(p_.depthMs,0,4);p_.rateHz=clamp(p_.rateHz,.05f,3);p_.wet=clamp(p_.wet,0,1);p_.spread=clamp(p_.spread,0,1); }
 void process(float* const* x,int channels,int frames){
  if(!x||frames<=0)return; const int nc=std::min(channels,ch_);
  const float base=static_cast<float>(p_.delayMs*.001*sr_), depth=static_cast<float>(p_.depthMs*.001*sr_);
  for(int n=0;n<frames;++n){
   float lfo[2]{};
   for(int c=0;c<nc;++c){ const float off=(c==1?.25f*p_.spread:0.0f); const float ph=phase_+off;
    lfo[c]=std::sin(2*float(M_PI)*ph)+.15f*std::sin(2*float(M_PI)*1.7f*ph+.15f);
    float dry=x[c][n]; if(!std::isfinite(dry))dry=0;
    auto &r=ring_[c]; r[write_]=dry;
    float d=base+depth*(lfo[c]+drift_); d=std::max(1.0f,std::min(float(r.size()-3),d));
    float pos=float(write_)-d; while(pos<0)pos+=float(r.size());
    int i0=int(pos); float t=pos-i0; int im=(i0-1+int(r.size()))%int(r.size()), i1=(i0+1)%int(r.size()), i2=(i0+2)%int(r.size());
    float y0=r[im],y1=r[i0],y2=r[i1],y3=r[i2];
    float a0=-.5f*y0+1.5f*y1-1.5f*y2+.5f*y3, a1=y0-2.5f*y1+2*y2-.5f*y3, a2=-.5f*y0+.5f*y2;
    float wet=((a0*t+a1)*t+a2)*t+y1;
    // Wet-copy HP 120 Hz and LP 8 kHz, one-pole.
    float hpA=std::exp(-2*float(M_PI)*120.0f/float(sr_)); float hp=hpA*(hpY_[c]+wet-hpX_[c]); hpX_[c]=wet;hpY_[c]=hp;
    float lpA=1-std::exp(-2*float(M_PI)*8000.0f/float(sr_)); lpY_[c]+=lpA*(hp-lpY_[c]);
    x[c][n]=dry*(1-p_.wet)+lpY_[c]*p_.wet;
   }
   rng_=1664525u*rng_+1013904223u; float noise=(float((rng_>>8)&0xffff)/32767.5f)-1.0f; drift_+=.00005f*(noise-drift_); drift_=std::max(-.5f,std::min(.5f,drift_));
   phase_+=p_.rateHz/float(sr_); if(phase_>=1)phase_-=1; write_=(write_+1)%ring_[0].size();
  }
 }
private:
 static float clamp(float x,float a,float b){return std::max(a,std::min(b,x));}
 double sr_=48000; int ch_=2; Params p_{}; std::array<std::vector<float>,2> ring_{}; size_t write_=0; float phase_=0,drift_=0; uint32_t rng_=1; std::array<float,2> hpX_{},hpY_{},lpY_{};
};
}
