#!/usr/bin/env python3
"""Butterworth design, SOS application and response reporting (v1.2.0)."""
__version__ = "1.2.0"
import argparse, warnings
import numpy as np
from scipy import signal

def _validate(fs, cutoff, btype):
    if not np.isfinite(fs) or fs <= 0: raise ValueError('fs must be finite and positive')
    c=np.asarray(cutoff,dtype=float)
    if not np.all(np.isfinite(c)) or np.any(c<=0) or np.any(c>=fs/2): raise ValueError('cutoff must lie strictly between 0 and Nyquist')
    if btype in ('bandpass','bandstop') and (c.size!=2 or c[0]>=c[1]): raise ValueError('band edges must be increasing')
    if btype in ('lowpass','highpass') and c.size!=1: raise ValueError('low/high pass requires one cutoff')

def design_sos(fs, cutoff, order=4, btype='lowpass'):
    _validate(fs,cutoff,btype)
    if int(order)<1: raise ValueError('order must be >= 1')
    return signal.butter(int(order),cutoff,btype=btype,fs=fs,output='sos')

def _clean(x):
    a=np.asarray(x)
    if not np.all(np.isfinite(a)):
        warnings.warn('non-finite input replaced with zero',RuntimeWarning)
        a=np.nan_to_num(a,copy=True,nan=0.0,posinf=0.0,neginf=0.0)
    return a

def apply_filter(x, b, a, axis=-1, dtype=None):
    y=signal.lfilter(b,a,_clean(x),axis=axis)
    return y.astype(dtype,copy=False) if dtype is not None else y

def apply_sos_filter(x,sos,axis=-1,dtype=None):
    y=signal.sosfilt(sos,_clean(x),axis=axis)
    return y.astype(dtype,copy=False) if dtype is not None else y

def report_sos_filter(sos,fs, worN=32768, label='SOS'):
    f,h=signal.sosfreqz(np.asarray(sos),worN=worN,fs=fs)
    db=20*np.log10(np.maximum(np.abs(h),1e-15))
    crossings=[]
    target=float(np.max(db)-3.01029995664)
    d=db-target
    for i in range(len(d)-1):
        if d[i]==0 or d[i]*d[i+1]<0:
            den=d[i+1]-d[i]; frac=0 if den==0 else -d[i]/den
            crossings.append(float(f[i]+frac*(f[i+1]-f[i])))
    print(f'{label}: {len(sos)} SOS sections; -3 dB crossings: '+(', '.join(f'{v:.3f} Hz' for v in crossings) or 'none'))
    return {'frequency_hz':f,'magnitude_db':db,'f_3db':tuple(crossings),'sos_sections':len(sos)}

def f_3db(b,a,fs,worN=32768):
    f,h=signal.freqz(b,a,worN=worN,fs=fs); db=20*np.log10(np.maximum(np.abs(h),1e-15)); target=np.max(db)-3.01029995664; d=db-target; out=[]
    for i in range(len(d)-1):
        if d[i]==0 or d[i]*d[i+1]<0:
            frac=0 if d[i+1]==d[i] else -d[i]/(d[i+1]-d[i]);out.append(float(f[i]+frac*(f[i+1]-f[i])))
    return tuple(out)

def main():
    p=argparse.ArgumentParser();p.add_argument('--fs',type=float,default=48000);p.add_argument('--cutoff',type=float,nargs='+',default=[3000]);p.add_argument('--order',type=int,default=4);p.add_argument('--type',choices=['lowpass','highpass','bandpass','bandstop'],default='lowpass');p.add_argument('--sos',action='store_true');p.add_argument('--report',action='store_true');p.add_argument('--clip',action='store_true');a=p.parse_args()
    cutoff=a.cutoff[0] if len(a.cutoff)==1 else a.cutoff;_validate(a.fs,cutoff,a.type);sos=design_sos(a.fs,cutoff,a.order,a.type)
    if a.report: report_sos_filter(sos,a.fs)
    else: print(sos)
    if a.clip: warnings.warn('--clip applies only when processing an audio array via the Python API; CLI is design/report only')
if __name__=='__main__':main()
