//
//  VoiceManager.h
//  Synthesis
//
//  Created by Devin Mooers on 1/21/14.
//
//

#ifndef __Synthesis__VoiceManager__
#define __Synthesis__VoiceManager__

#include <iostream>
#include "Voice.h"
#include <boost/array.hpp>
#include "OpenCL.h"

class VoiceManager {
public:
    static VoiceManager& getInstance() {
        static VoiceManager theInstance;
        return theInstance;
    };
    void onNoteOn(int noteNumber, int velocity);
    void onNoteOff(int noteNumber, int velocity);
    void onSustainChange(double sustain);
    void onExpressionChange(double expression);
    void onModChange(double mod);
    void setSampleRate(double sampleRate);
    void updateInharmonicityCoeff(float mB) {
        mOpenCL.mB = mB;
    }
    void updateNumPartials(int partials) {
        mOpenCL.NUM_PARTIALS = partials;
    }
    void updateStringDetuneRange(float val) {
        mOpenCL.mStringDetuneRange = val;
    }
    void updatePartialDetuneRange(float val) {
        mOpenCL.mPartialDetuneRange = val;
    }
    void updateDamping(float val) {
        mOpenCL.mDamping = val;
    }
    void updateLinearTerm(float val) {
        mOpenCL.instrumentData[0] = val;
    }
    void updateSquaredTerm(float val) {
        mOpenCL.instrumentData[1] = val;
    }
    void updateCubicTerm(float val) {
        mOpenCL.instrumentData[2] = val;
    }
    void updateBrightnessA(float val) {
        mOpenCL.instrumentData[3] = val;
    }
    void updateBrightnessB(float val) {
        mOpenCL.instrumentData[4] = val;
    }
    void updatePitchBendCoarse(float val) {
        mOpenCL.instrumentData[5] = val;
    }
    void updatePitchBendFine(float val) {
        mOpenCL.instrumentData[6] = val;
    }
    boost::array<double, BLOCK_SIZE*NUM_CHANNELS> getBlockOfSamples();
    inline void initOpenCL() {
        zeroes.assign(0.0);
        currentEnergySampleIndex = 0;
        mOpenCL.initOpenCL();
    }
    void updateVoiceDampingAndEnergy(int i);
    void setFreeInaudibleVoices();
    OpenCL mOpenCL;

private:
    /* No instantiation from outside (i.e. singleton) */
    VoiceManager() {};
    /* Explicitly disallow copying: */
    VoiceManager(const VoiceManager&);
    VoiceManager& operator= (const VoiceManager&);
    Voice voices[MAX_VOICES];
    int getNumberOfActiveVoices();
    int numActiveVoices;
    int currentEnergySampleIndex;
    void updateVoiceData(); // this is called on every sample to update damping and energy values
    Voice* findVoicePlayingSameNote(int noteNumber);
    Voice* findFreeVoice();
    Voice* findOldestVoice();
    boost::array<double, BLOCK_SIZE*NUM_CHANNELS> zeroes; // zero-samples for returning if no active voices
};

#endif /* defined(__Synthesis__VoiceManager__) */
