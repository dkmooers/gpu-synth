//
//  Filter.h
//  Synthesis
//
//  Created by Devin Mooers on 2/7/14.
//
//

#ifndef __Synthesis__Filter__
#define __Synthesis__Filter__

class Filter {
public:
    enum FilterMode {
        FILTER_MODE_LOWPASS = 0,
        FILTER_MODE_HIGHPASS,
        FILTER_MODE_BANDPASS,
        kNumFilterModes
    };
    Filter() :
    cutoff(0.5),
    resonance(0.0),
    mode(FILTER_MODE_BANDPASS),
    buf0(0.0),
    buf1(0.0),
    buf2(0.0),
    buf3(0.0)
    {
        calculateFeedbackAmount();
    };
    double process(double inputValue);
    inline void setCutoff(double newCutoff) { cutoff = newCutoff; calculateFeedbackAmount(); };
    inline void setResonance(double newResonance) { resonance = newResonance; calculateFeedbackAmount(); };
    inline void setFilterMode(FilterMode newMode) { mode = newMode; }
    private:
    double cutoff;
    double resonance;
    FilterMode mode;
    double feedbackAmount;
    inline void calculateFeedbackAmount() { feedbackAmount = resonance + resonance/(1.0 - cutoff); }
    double buf0;
    double buf1;
    double buf2;
    double buf3;
};

#endif /* defined(__Synthesis__Filter__) */
