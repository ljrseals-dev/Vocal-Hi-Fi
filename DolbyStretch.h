#pragma once
#include <array>
#include <cmath>
#include <algorithm>
namespace vh {
class DolbyStretch {
public:
 struct Params { bool enabled=true; float mix=.55f, trimDb=-1.0f; bool stretch=true; };
 void prepare(double sr){sr_=sr; reset();}
 void reset(){low_.fill(0);mid_.fill(0);hi_.fill(0);env_.fill(0);}
 void setParams(const Params&p){p_=p;p_.mix=clamp(p_.mix,0,1);p_.trimDb=clamp(p_.trimDb,-12,6);}
 float process(float x,int ch){ if(!std::isfinite(x)) { resetChannel(ch); return 0; } if(!p_.enabled)return x;
  ch=std::max(0,std::min(1,ch));
  // First-order complementary splits at 80 Hz, 3 kHz and 9 kHz.
  float b0=onepole(x,low_[ch],80), r0=x-b0;
  float b1=onepole(r0,mid_[ch],3000), r1=r0-b1;
  float b2=onepole(r1,hi_[ch],9000), b3=r1-b2;
  float bands[4]={b0,b1,b2,b3};
  const float thDb[4]={-32,-32,-34,-38};
  const float boostDb[4]={8.0f,p_.stretch?12.0f:10.0f,p_.stretch?16.0f:10.0f,p_.stretch?22.0f:12.0f};
  const float ratio[4]={2,p_.stretch?2.5f:2,p_.stretch?3.0f:2,p_.stretch?3.5f:2};
  float y=0;
  for(int i=0;i<4;++i){ float mag=std::fabs(bands[i]); float atkMs[4]={4,3,2,1}, relMs[4]={60,45,30,20};
   float coef=std::exp(-1.0f/(float(sr_)*(mag>env_[i*2+ch]?atkMs[i]:relMs[i])*.001f));
   env_[i*2+ch]=coef*env_[i*2+ch]+(1-coef)*mag;
   float threshold=std::pow(10.0f,thDb[i]/20.0f); float maxG=std::pow(10.0f,boostDb[i]/20.0f);
   // Upward expansion below threshold; bounded maximum boost. Envelope smoothing prevents pumping.
   float g=1.0f; if(env_[i*2+ch]<threshold){ float q=std::pow(std::max(env_[i*2+ch],1.0e-9f)/threshold,1.0f/ratio[i]-1.0f); g=std::min(maxG,std::max(1.0f,q)); }
   y+=bands[i]*g;
  }
  float trim=std::pow(10.0f,p_.trimDb/20.0f); float wet=y*trim; return x+(wet-x)*p_.mix;
 }
private:
 static float clamp(float x,float a,float b){return std::max(a,std::min(b,x));}
 float onepole(float x,float &z,float hz){float a=1-std::exp(-2*float(M_PI)*hz/float(sr_));z+=a*(x-z);return z;}
 void resetChannel(int ch){ch=std::max(0,std::min(1,ch));low_[ch]=mid_[ch]=hi_[ch]=0;for(int i=0;i<4;++i)env_[i*2+ch]=0;}
 double sr_=48000; Params p_{}; std::array<float,2> low_{},mid_{},hi_{}; std::array<float,8> env_{};
};
}
