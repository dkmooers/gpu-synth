//
//  Filter.cpp
//  Synthesis
//
//  Created by Devin Mooers on 2/7/14.
//
//

#include "Filter.h"

// By Paul Kellett
// http://www.musicdsp.org/showone.php?id=29

double Filter::process(double inputValue) {
    buf0 += cutoff * (inputValue - buf0);
    buf1 += cutoff * (buf0 - buf1);
    switch (mode) {
        case FILTER_MODE_LOWPASS:
        return buf1;
        case FILTER_MODE_HIGHPASS:
        return inputValue - buf0;
        case FILTER_MODE_BANDPASS:
        return buf0 - buf1;
        default:
        return 0.0;
    }
//    buf0 += cutoff * (inputValue - buf0);
//    buf1 += cutoff * (buf0 - buf1);
//    buf2 += cutoff * (buf1 - buf0);
//    buf3 += cutoff * (buf2 - buf1);
//    switch (mode) {
//        case FILTER_MODE_LOWPASS:
//        return buf3;
//        case FILTER_MODE_HIGHPASS:
//        return inputValue - buf3;
//        case FILTER_MODE_BANDPASS:
//        return buf0 - buf3;
//        default:
//        return 0.0;
//    }
}