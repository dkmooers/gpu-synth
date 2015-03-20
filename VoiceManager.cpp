//
//  VoiceManager.cpp
//  Synthesis
//
//  Created by Devin Mooers on 1/21/14.
//
//

#include "VoiceManager.h"

int VoiceManager::getNumberOfActiveVoices() {
    int count = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].isActive) {
            count++;
        }
    }
    return count;
}

Voice* VoiceManager::findVoicePlayingSameNote(int noteNumber) {
    Voice* sameNoteVoice = NULL;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].mNoteNumber == noteNumber) {
            sameNoteVoice = &(voices[i]);
            break;
        }
    }
    return sameNoteVoice;
}

Voice* VoiceManager::findFreeVoice() {
    Voice* freeVoice = NULL;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].isActive) {
            freeVoice = &(voices[i]);
            break;
        }
    }
    return freeVoice;
}

Voice* VoiceManager::findOldestVoice() {
    double age = 0.0;
    int indexOfOldest = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].mTime > age) {
            age = voices[i].mTime;
            indexOfOldest = i;
        }
    }
    return &(voices[indexOfOldest]);
}

void VoiceManager::onNoteOn(int noteNumber, int velocity) {
    // print number of active voices
    //std::cout << "\nActive voices = " << getNumberOfActiveVoices();
    // first look for a voice that's playing the same note
//    Voice* voice = findVoicePlayingSameNote(noteNumber);
//    // if no voice playing same note, look for a free voice
//    if (!voice) {
//        voice = findFreeVoice();
//    }
    
    Voice* voice = findFreeVoice();
    // then do voice stealing
    if (!voice) {
        voice = findOldestVoice();
        //printf("stole a voice!");
    }
    
    /// velocity scaling (scaling from int (0, 127) to float (0,1) WITH velocity curve)
    //float scaledVelocity = 0.14948f * powf(1.055f, 0.3f*velocity)-0.14948f; // velocity scaling to (0, 1) w/velocity curve
    
    float scaledVelocity = velocity / 127.0f;
    
    voice->reset();
    voice->setNoteNumber(noteNumber);
    //voice->mDamping = ((float)noteNumber/100.0f)*2.5f; /// set this to be a param amount set by a knob (and modified by expression pedal) to control decay time
    voice->mDamping = ((float)noteNumber/100.0f)*mOpenCL.mDamping;
    voice->lastExcitationTimeAgo = 0.0f;
    voice->lastExcitationStrength = scaledVelocity;
    voice->lastExcitationDuration = 0.005f; // 5ms
    voice->isActive = true;
    voice->mVelocity = scaledVelocity;
    
    voice->mEnergyHoriz *= (1-scaledVelocity); // louder hits will "reset" the velocity more - a full loudness hit will totally reset the string back to zero energy
    voice->mEnergyVert *= (1-scaledVelocity);
    
    //voice->mEnergyHoriz = scaledVelocity;
    //voice->mEnergyVert = scaledVelocity;
    //printf("---NOTE ON---\n");
    //voice->mEnergy = scaledVelocity; /// actually this should also be a RAMP function so we get a smooth ramping up to the target mEnergy value
    voice->mStringDetuneAmount = (1.0f-mOpenCL.mStringDetuneRange) + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(2.0f*mOpenCL.mStringDetuneRange)));
    voice->randomSeed = rand() % 10000+1000; // set random seed on each note hit for randomizing partial frequencies and amplitudes
}

void VoiceManager::onNoteOff(int noteNumber, int velocity) {
    for (int i = 0; i < MAX_VOICES; i++) {
        Voice& voice = voices[i];
        if (voice.isActive && voice.mNoteNumber == noteNumber) {
            //voice.mDamping = 50.0f; /// actually, this should be a RAMP function, which takes target mDamping value and # of samples it'll take to get there (or SPEED), and then the function increases damping gradually (iterates over several samples of the damping array, perhaps? how to handle this?)
        }
    }
}

void VoiceManager::onSustainChange(double sustain) {
    mOpenCL.MIDIParams[0] = (float)sustain;
}

void VoiceManager::onExpressionChange(double expression) {
    mOpenCL.MIDIParams[1] = (float)expression;
}

void VoiceManager::onModChange(double mod) {
//    mOpenCL.MIDIParams[2] = (float)mod;
    mOpenCL.mModCurrent = (float)mod;
}

void VoiceManager::setSampleRate(double sampleRate) {
}

void VoiceManager::updateVoiceData() {
    int j = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        Voice& voice = voices[i];
        if (voice.isActive) {
            mOpenCL.voicesData[j*NUM_VOICE_PARAMS]   = voice.mTime;
            mOpenCL.voicesData[j*NUM_VOICE_PARAMS+1] = voice.mFrequency;
            mOpenCL.voicesData[j*NUM_VOICE_PARAMS+2] = voice.mVelocity;
            mOpenCL.voicesData[j*NUM_VOICE_PARAMS+3] = voice.mStringDetuneAmount;
            mOpenCL.voicesData[j*NUM_VOICE_PARAMS+4] = voice.randomSeed;
            j++;
            voice.mTime += mOpenCL.mTimeStep * BLOCK_SIZE;
        }
    }
}

void VoiceManager::setFreeInaudibleVoices() {
    for (int i = 0; i < MAX_VOICES; i++) {
        Voice& voice = voices[i];
        if (voice.isActive) {
            if ((voice.mEnergyHoriz + voice.mEnergyVert) <= voice.mBrownianThreshold) {
                voice.setFree();
//                printf("voice %d set free!\n", i);
            }
        }
    }
}

void VoiceManager::updateVoiceDampingAndEnergy(int currentSampleIndex) {
    int j = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        Voice& voice = voices[i];
        if (voice.isActive) {
            float duration = voice.lastExcitationDuration;
            float timeAgo = voice.lastExcitationTimeAgo;
            float timeStep = mOpenCL.mTimeStep;
            if (timeAgo < duration) {
                float deltaEnergy = timeStep * static_cast<float>( exp( - ( pow(timeAgo - duration/2.0f, 2) / (duration*duration*0.03f) ) ) ) * voice.lastExcitationStrength * 500.0f; // smooth ramp for energy // gaussian distribution force function that starts increasing immediately after strike and goes back down to zero after exactly duration seconds. the 0.03f is so that the gaussian curve just touches zero at the beginning and end of the transient
                voice.mEnergyVert += deltaEnergy * (1.0f-voice.mHorizToVertRatio); // this was just 1.0, with 250.0f final multiplier in above line
                voice.mEnergyHoriz += deltaEnergy * voice.mHorizToVertRatio;
                //printf("timeAgo = %f, duration = %f, voice[%d] energyVert = %f, energyHoriz = %f\n", timeAgo, duration, i, voice.mEnergyVert, voice.mEnergyHoriz);
            }
            if (currentSampleIndex == 0) {
                //printf("timeAgo = %f, duration = %f, voice[%d] energyVert = %f, energyHoriz = %f\n", timeAgo, duration, i, voice.mEnergyVert, voice.mEnergyHoriz);
            }
            voice.mEnergyVert -= voice.mEnergyVert * timeStep * voice.mDamping * (1.0f-voice.mHorizToVertRatio);
            voice.mEnergyHoriz -= voice.mEnergyHoriz * timeStep * voice.mDamping * (voice.mHorizToVertRatio);
            mOpenCL.voicesEnergy[j*BLOCK_SIZE + currentSampleIndex] = voice.mEnergyVert + voice.mEnergyHoriz; // set voice energy with sum of horizontal and vertical modes of string vibration
            voice.lastExcitationTimeAgo += mOpenCL.mTimeStep;
            j++;
        }
    }
}

boost::array<double, BLOCK_SIZE * NUM_CHANNELS> VoiceManager::getBlockOfSamples() {
    
    numActiveVoices = getNumberOfActiveVoices();
    mOpenCL.NUM_ACTIVE_VOICES = numActiveVoices;
    //std::cout << "\nactive voices: " << numActiveVoices;
    if (numActiveVoices == 0) {
        return zeroes;
    } else {
        updateVoiceData(); // writes new voice data to mOpenCL
        return mOpenCL.getBlockOfSamples();
    }
}
