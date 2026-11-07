#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void* vp_create(double sampleRate,int channels);void vp_destroy(void*);void vp_reset(void*);void vp_set_param(void*,int,float);float vp_get_param(void*,int);int vp_param_count(void);void vp_param_info(int,const char**,float*,float*,float*);void vp_process_planar(void*,float**,int,int);void vp_process_interleaved(void*,float*,int,int);int vp_latency_samples(void);int vp_load_preset(void*,const char*);
#ifdef __cplusplus
}
#endif
