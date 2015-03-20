//
//  Voice.cpp
//  Synthesis
//
//  Created by Devin Mooers on 1/21/14.
//
//

#include "Voice.h"

void Voice::setFree() {
    isActive = false;
    reset();
}

void Voice::reset() {
    mNoteNumber = -1;
    mVelocity = 0.0f;
    mTime = 0.0f;
    //mOscillator.reset();
}