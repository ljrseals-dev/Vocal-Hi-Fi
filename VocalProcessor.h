#pragma once
#include "ADT.h"
#include "DolbyStretch.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cstring>

namespace vh {
class VocalProcessor {
public:
 struct Settings {
  float highpassHz=80,mudHz=300,mudQ=1,mudDb=-2,presenceHz=3000,presenceQ=.9f,presenceDb=3,airHz=10000,airDb=2,notchHz=2600,notchQ=4,notchDb=-1.5f;
  float warmth=.2f,deessHz=6500,deessThreshDb=-26,deessRatio=4,deessAmount=.5f,compThreshDb=-18,compRatio=2.5f,compAttackMs=8,compReleaseMs=120;
  bool dolbyStretch=true; float dolbyMix=.55f,dolbyTrimDb=-1; float breathHz=4500,breathAmount=.05f; float outputGainDb=0,mix=1; bool softClip=true;
  float adtDelayMs=22,adtDepthMs=1.5f,adtRateHz=.35f,adtWet=.35f,adtSpread=.85f;
 };
 void prepare(double sr,int channels=2){sr_=std::max(8000.0,std::min(192000.0,sr));ch_=std::max(1,std::min(2,channels));adt_.prepare(sr_,ch_);dolby_.prepare(sr_);reset();}
 void setSettings(const Settings&s){ pending_=s; dirty_.store(true,std::memory_order_release); }
 void reset(){for(auto &c:st_)c={}; adt_.reset();dolby_.reset();gainEnv_.fill(0);}
 void process(float* const* channels,int nCh,int frames){
  if(!channels||frames<=0)return; if(dirty_.exchange(false,std::memory_order_acq_rel)){ active_=pending_; ADT::Params ap;ap.delayMs=active_.adtDelayMs;ap.depthMs=active_.adtDepthMs;ap.rateHz=active_.adtRateHz;ap.wet=active_.adtWet;ap.spread=active_.adtSpread;adt_.setParams(ap);DolbyStretch::Params dp;dp.enabled=active_.dolbyStretch;dp.mix=active_.dolbyMix;dp.trimDb=active_.dolbyTrimDb;dolby_.setParams(dp); }
  const int nc=std::min(nCh,ch_); float outG=std::pow(10.0f,clamp(active_.outputGainDb,-36,18)/20.0f);
  for(int n=0;n<frames;++n)for(int c=0;c<nc;++c){float x=channels[c][n];if(!std::isfinite(x)){x=0;st_[c]={};dolby_.reset();}
   // DC block and Butterworth-like HP (two cascaded one-pole sections).
   x=onepoleHP(x,st_[c].dc,5); x=onepoleHP(x,st_[c].hp1,active_.highpassHz); x=onepoleHP(x,st_[c].hp2,active_.highpassHz);
   x+=biquad(x,st_[c].mud,active_.mudHz,active_.mudQ,active_.mudDb,false);
   x+=biquad(x,st_[c].pres,active_.presenceHz,active_.presenceQ,active_.presenceDb,true);
   x+=shelf(x,st_[c].air,active_.airHz,active_.airDb);
   x+=biquad(x,st_[c].notch,active_.notchHz,active_.notchQ,active_.notchDb,false);
   float w=clamp(active_.warmth,0,1); float sat=std::tanh(x*(1+w*2.5f))+.18f*std::tanh(x*.6f); x+=(sat-x)*.35f;
   float hf=onepoleHP(x,st_[c].deess,active_.deessHz); float env=std::fabs(hf); float a=std::exp(-1.0f/(float(sr_)*(env>gainEnv_[c]?.002f:.04f)));gainEnv_[c]=a*gainEnv_[c]+(1-a)*env;
   float th=std::pow(10.0f,active_.deessThreshDb/20.0f); float reduction=1; if(gainEnv_[c]>th) reduction=std::pow(th/std::max(gainEnv_[c],1e-9f),1-1/clamp(active_.deessRatio,1,20));
   x-=hf*(1-reduction)*clamp(active_.deessAmount,0,1);
   x=dolby_.process(x,c);
   float peak=std::fabs(x); float cAtk=std::exp(-1.0f/(float(sr_)*clamp(active_.compAttackMs,0.1f,200)*.001f));float cRel=std::exp(-1.0f/(float(sr_)*clamp(active_.compReleaseMs,1,2000)*.001f));float ca=peak>st_[c].compEnv?cAtk:cRel;st_[c].compEnv=ca*st_[c].compEnv+(1-ca)*peak;
   float ct=std::pow(10.0f,active_.compThreshDb/20.0f); if(st_[c].compEnv>ct){float g=std::pow(ct/st_[c].compEnv,1-1/clamp(active_.compRatio,1,20));x*=g;}
   float breath=std::fabs(x); if(breath<.03f){float h=onepoleHP(x,st_[c].breath,active_.breathHz);x+=h*clamp(active_.breathAmount,0,.05f);}
   x*=outG; x*=clamp(active_.mix,0,1);
   if(active_.softClip && std::fabs(x)>.9f)x=.9f*std::tanh(x/.9f);
   if(!std::isfinite(x))x=0; if(std::fabs(x)<1e-25f)x=0; channels[c][n]=x;
  }
  // ADT is applied after the per-channel chain. This preserves wet decorrelation.
  adt_.process(channels,nc,frames);
 }
 void processInterleaved(float* b,int nc,int nf){if(!b||nc<1||nc>2||nf<1)return; for(int c=0;c<nc;++c)for(int n=0;n<nf;++n)interleaved_[c][n]=b[n*nc+c];float* p[2]={interleaved_[0].data(),interleaved_[1].data()};process(p,nc,nf);for(int c=0;c<nc;++c)for(int n=0;n<nf;++n)b[n*nc+c]=p[c][n];}
 static int latencySamples(){return 0;}
private:
 struct State{float dc=0,hp1=0,hp2=0,deess=0,compEnv=0,breath=0; struct B{float x1=0,x2=0,y1=0,y2=0;}mud,pres,notch;float air=0;};
 static float clamp(float x,float a,float b){return std::max(a,std::min(b,x));}
 float onepoleHP(float x,float&z,float hz){float a=std::exp(-2*float(M_PI)*hz/float(sr_));z=(1.0f-a)*x+a*z;return x-z;}
 float biquad(float x,State::B&s,float hz,float q,float db,bool peak){double A=std::pow(10.0,db/40.0),w=2*M_PI*clamp(hz,20,float(sr_*.45))/sr_,cs=std::cos(w),sn=std::sin(w),alpha=sn/(2*std::max(.1f,q));double b0,b1,b2,a0,a1,a2;if(peak){b0=1+alpha*A;b1=-2*cs;b2=1-alpha*A;a0=1+alpha/A;a1=-2*cs;a2=1-alpha/A;}else{b0=1+alpha*A;b1=-2*cs;b2=1-alpha*A;a0=1+alpha/A;a1=-2*cs;a2=1-alpha/A;}float y=float((b0/a0)*x+(b1/a0)*s.x1+(b2/a0)*s.x2-(a1/a0)*s.y1-(a2/a0)*s.y2);s.x2=s.x1;s.x1=x;s.y2=s.y1;s.y1=y;return y-x;}
 float shelf(float x,float&z,float hz,float db){float a=1-std::exp(-2*float(M_PI)*clamp(hz,100,float(sr_*.45))/float(sr_));z+=a*(x-z);return z*(std::pow(10.0f,db/20.0f)-1);}
 double sr_=48000;int ch_=2;Settings active_{},pending_{};std::atomic<bool>dirty_{false};std::array<State,2>st_{};std::array<float,2>gainEnv_{};ADT adt_;DolbyStretch dolby_;std::array<std::vector<float>,2> interleaved_{{std::vector<float>(65536),std::vector<float>(65536)}};
};
}
