#include "VocalProcessor.h"
#include <new>
#include <cstring>
#include <string>
#include <cstdlib>

struct VPHandle { vh::VocalProcessor p; vh::VocalProcessor::Settings s; };
extern "C" {
void* vp_create(double sr,int ch){try{auto*h=new VPHandle;h->p.prepare(sr,ch);return h;}catch(...){return nullptr;}}
void vp_destroy(void*h){delete static_cast<VPHandle*>(h);}
void vp_reset(void*h){if(h)static_cast<VPHandle*>(h)->p.reset();}
void vp_set_param(void*h,int id,float v){if(!h||!std::isfinite(v))return;auto*q=static_cast<VPHandle*>(h);switch(id){case 0:q->s.highpassHz=v;break;case 1:q->s.mudDb=v;break;case 2:q->s.presenceDb=v;break;case 3:q->s.airDb=v;break;case 4:q->s.warmth=v;break;case 5:q->s.deessAmount=v;break;case 6:q->s.dolbyMix=v;break;case 7:q->s.outputGainDb=v;break;case 8:q->s.adtWet=v;break;default:return;}q->p.setSettings(q->s);}
float vp_get_param(void*h,int id){if(!h)return 0;auto&s=static_cast<VPHandle*>(h)->s;switch(id){case 0:return s.highpassHz;case 1:return s.mudDb;case 2:return s.presenceDb;case 3:return s.airDb;case 4:return s.warmth;case 5:return s.deessAmount;case 6:return s.dolbyMix;case 7:return s.outputGainDb;case 8:return s.adtWet;default:return 0;}}
int vp_param_count(){return 9;}
void vp_param_info(int id,const char**name,float*mn,float*mx,float*def){static const char*n[]={"highpass_hz","mud_db","presence_db","air_db","warmth","deess_amount","dolby_mix","output_gain_db","adt_wet"};static const float lo[]={20,-12,-12,-12,0,0,0,-36,0},hi[]={500,12,12,12,1,1,1,18,1},dv[]={80,-2,3,2,.2f,.5f,.55f,0,.35f};if(id<0||id>=9)return;if(name)*name=n[id];if(mn)*mn=lo[id];if(mx)*mx=hi[id];if(def)*def=dv[id];}
void vp_process_planar(void*h,float**ch,int nc,int nf){if(h)static_cast<VPHandle*>(h)->p.process(ch,nc,nf);}
void vp_process_interleaved(void*h,float*b,int nc,int nf){if(h)static_cast<VPHandle*>(h)->p.processInterleaved(b,nc,nf);}
int vp_latency_samples(){return 0;}
int vp_load_preset(void*h,const char*json){if(!h||!json)return 0; // Deliberately accepts the bundled preset names via a compact, dependency-free key reader.
 auto*q=static_cast<VPHandle*>(h);std::string s(json);auto read=[&](const char*k,float&v){auto p=s.find(std::string("\"")+k+"\"");if(p==std::string::npos)return; p=s.find(':',p);if(p==std::string::npos)return;char*e=nullptr;float x=std::strtof(s.c_str()+p+1,&e);if(e!=s.c_str()+p+1&&std::isfinite(x))v=x;};read("highpassHz",q->s.highpassHz);read("mudDb",q->s.mudDb);read("presenceDb",q->s.presenceDb);read("airDb",q->s.airDb);read("warmth",q->s.warmth);read("deessAmount",q->s.deessAmount);read("dolbyMix",q->s.dolbyMix);read("dolbyTrimDb",q->s.dolbyTrimDb);read("outputGainDb",q->s.outputGainDb);q->p.setSettings(q->s);return 1;}
}
