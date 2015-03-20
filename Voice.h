//
//  Voice.h
//  Synthesis
//
//  Created by Devin Mooers on 1/21/14.
//
//

#ifndef __Synthesis__Voice__
#define __Synthesis__Voice__

#include <iostream>
#include "Oscillator.h"
#include <boost/array.hpp>

class Voice {
    
public:
    friend class VoiceManager;
    // constructor:
    Voice()
    : mNoteNumber(-1),
    mVelocity(0.0f),
    mEnergyVert(0.0f),
    mEnergyHoriz(0.0f),
    mHorizToVertRatio(0.3f),
    mDamping(0.0f),
    mBrownianThreshold(0.00001),
    lastExcitationTimeAgo(0.0f),
    lastExcitationDuration(0.0f),
    lastExcitationStrength(0.0f),
    isActive(false) {}
    // public member functions:
    inline void setNoteNumber(int noteNumber) {
        mNoteNumber = noteNumber;
        mFrequency = 440.0f * powf(2.0f, (mNoteNumber - 69.0f) / 12.0f);
        mOscillator.setFrequency(mFrequency);
    }
    void setFree();
    void reset();
    
private:
    Oscillator mOscillator;
    int mNoteNumber;
    float mTime; /// time counter for this voice, since last voice reset()
    float mEnergyVert; // energy stored in the vertical mode of vibration for this voice (decays faster)
    float mEnergyHoriz; // energy stored in the horizontal mode of vibration for this voice (decays slower)
    float mHorizToVertRatio; /// I could generalize this later to be, instead of vert and horiz, do an arbitrary number of axes of vibrations that create an N-stage amplitude decay! // range (0, 1), dictates how much slower horiz energy decays compared to vert energy (based on differing admittances at the string bridge) --> 0.5 indicates they decay at the same rate, while 0.1 means vert decays 10 times faster
    float mDamping; // current damping value for this voice
    float mBrownianThreshold; // minimum mEnergy value below which we'll reset the voice
    float mFrequency;
    float mVelocity;
    float mStringDetuneAmount;
    float randomSeed; // stored as float b/c passing in as float to kernel
    float lastExcitationTimeAgo; // how many samples ago the last excitation occurred for this voice
    float lastExcitationDuration; // in samples /// WARNING: this may cause an error on sample rate switch... or just audible artifacts... maybe ok
    float lastExcitationStrength;
    bool isActive;
};

#endif /* defined(__Synthesis__Voice__) */
