//
//  Oscillator.h
//  Synthesis
//
//  Created by Devin Mooers on 1/12/14.
//
//

#ifndef __Synthesis__Oscillator__
#define __Synthesis__Oscillator__

#include <iostream>
#include <math.h>
#include <vector>
#include "Signal.h"
#include <boost/array.hpp>



class Oscillator {
    
public:
    enum OscillatorMode {
        OSCILLATOR_MODE_SINE,
        OSCILLATOR_MODE_SAW,
        OSCILLATOR_MODE_SQUARE,
        OSCILLATOR_MODE_TRIANGLE,
        kNumOscillatorModes
    };
    void setMode(OscillatorMode mode);
    void setFrequency(double frequency);
    void setVelocity(int velocity);
    void setPitchMod(double amount);
    void setSampleRate(double sampleRate);
    void updateInstrumentModel();
    void updateInharmonicityCoeff(double mBNew);
    void updateNumPartials(int mNumPartialsNew);
    inline double getTime() { return mTime; }
    //inline void setMuted(bool muted) { isMuted = muted; }
    inline void resetTime() { mTime = 0.0; } // resets time counter whenever a new noteOn signal is triggered
    inline void reset() { mTime = 0.0; updateInstrumentModel(); };
    boost::array<double, 2> nextSample();

    void generate(double* buffer, int nFrames);

    
    Oscillator() : // this is the constructor's "initializer list"
        mOscillatorMode(OSCILLATOR_MODE_SINE),
        mPI(2*acos(0.0)),
        twoPI(2 * mPI),
        mTimeStep(1.0 / 44100.0),

        mNumStringsPerNote(2),
    
        mStringDetuneRange(0.001), // .002 is good
        mStringRatio(1.0), // calculated dynamically based on mStringDetuneRange (value here doesn't matter)
        mB(0.0001), // sample inharmonic coefficient for A2 piano string, default .00012, .0012 sounds super cool! .0002 is subtler
    
        // for lorenz
        mLX(0.0),
        mLXdot(0.0),
        mLY(1.0),
        mLYdot(0.0),
        mLZ(1.05),
        mLZdot(0.0),
        mLSigma(10.0),
        mLRho(24.0), // default 28.0
        mLBeta(8.0/3.0),
        mLRand(1.0),
    
        // for tinkerbell map
        mTX(-0.72),
        mTY(-0.64),
        mTXnew(0.0),
        mTYnew(0.0),
        mTa(0.9),
        mTb(-0.6013),
        mTc(2.0),
        mTd(0.50),
    
        mCalculateNoisyTransient(true),
        mAttack(3000), // the higher, the shorter the attack. 3000 is good soft noisy attack
        mTransientLength(0.500), // length in seconds to continue calculating transients for
        mPartialsDelay(0.005), // length in seconds to delay partial synthesis after note trigger (so it starts slightly after hit/pluck/excitation/transient)

        // string hit location stuff
        mStringHitLocation(0.166),
        mLocationBasedAmplitude(1.0),
    
        mFrequency(440.0),
        mLastFrequency(220.0), // could be anything, just needs to be different than mFrequency for starters, or else comparison returns true on first note strike after program init (and we want it to return false)
        mVelocity(0),
        mPitchMod(0.0),
        mNumPartials(30),
        mMaxFreq(20000.0), // eventually might want to limit this to human hearing for performance reasons - will be generating partials way above human hearing at 96k, 192k, etc. sample rates
        mMaxPartials(500.0), // max number of partials possible (governs things like array length for storing slightly-inharmonic partial frequencies)
        //mPartialInharmonicityCoeffs({ })
        mPartialFrequency(0.0),
        mAmplitude(0.0),
        mPartialInharmonicityCoeff(1.0)
        {
            updateTime();
            updateInstrumentModel();
            seedRNG();
        };
    
private:
    
    OscillatorMode mOscillatorMode;
    const double mPI;
    const double twoPI;
    const double mMaxFreq; // cutoff frequency, above which do not generate any partials (for human hearing limits and performance)
    const double mMaxPartials;
    const double mNumStringsPerNote;
    double mB; // inharmonicity coefficient --> B = pi^2 * Q * S * K^2 / (T * l^2) --> see Evernote "Research" for details
    
    int mVelocity;
    double mPitchMod;
    
    double mFrequency;
    double mLastFrequency; // for storing the last frequency
    static double mSampleRate;
    double mStringRatio; // ratio of frequencies of one string to the other (per note)
    double mPartialInharmonicityCoeff; // calculated dynamically for each partial in nextSample()
    double mPartialFrequencyMultipliers [1000]; // twice as many as inharmonicitycoeffs b/c allowing for two strings (double the number of partials)
    double mPartialFrequencies [1000]; // just calculating partial freq directly!
    double mPartialAmplitudes [1000]; // same as above
    double mPartialPanValues [1000];
    double mFrequencies [10]; // allows for up to X strings per note
    double mPartialFrequency;
    double mTime;
    double mTimeStep; // seconds per sample (for keeping running total of time)
    //unsigned long long mSampleIndex; // running total of which sample index we're on, reset at every note trigger. should be good for 13.26 million years of samples at 44.1kHz
    double mAmplitude; // used for calculating partial amplitudes
    
    // strange attractor variables
    double mLX;
    double mLXdot;
    double mLY;
    double mLYdot;
    double mLZ;
    double mLZdot;
    double mLSigma;
    double mLRho;
    double mLBeta;
    double mLRand;
    
    // for Tinkerbell map
    double mTX;
    double mTXnew;
    double mTY;
    double mTYnew;
    double mTa;
    double mTb;
    double mTc;
    double mTd;
    
    double mCalculateNoisyTransient; // whether or not to calculate noise
    double mAttack;
    double mTransientLength;
    double mPartialsDelay; // delay partial synthesis by a small amount of time, to simulate the attack/hammer/pluck/excitation happening slightly before the actual partials sounding
    
    double values[2]; // temporary array for summing partials together
    boost::array<double, 2> samples; // container for returning samples (or blocks of samples, when I start using OpenCL)
    
    double noise; // var for calculating chaos noise
    
    double mStringHitLocation;
    double mLocationBasedAmplitude;
    
    double mNumPartials; // probably don't need double precision on this! doing float instead of int b/c need to divide by float when calculating partial amplitude, couldn't figure out any other way to do it
    double mStringDetuneRange; // multiplier for slightly detuning individual strings for the same note
    
    
    void getLocationBasedAmplitude(int i);
    void updateTime();
    inline void seedRNG() { srand(static_cast<unsigned>(time(0))); } // seed random generator, once per program run
    
};

#endif /* defined(__Synthesis__Oscillator__) */
