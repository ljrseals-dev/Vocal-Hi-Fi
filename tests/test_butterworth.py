import numpy as np
from butterworth_filter import design_sos, report_sos_filter

def test_crossing_counts():
    fs=48000
    cases=[('lowpass',4000,1),('highpass',4000,1),('bandpass',[500,5000],2),('bandstop',[500,5000],2)]
    for typ,cut,n in cases:
        r=report_sos_filter(design_sos(fs,cut,4,typ),fs,worN=65536)
        assert len(r['f_3db'])==n
