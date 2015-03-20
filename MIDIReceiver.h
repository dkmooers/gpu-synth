//
//  MIDIReceiver.h
//  Synthesis
//
//  Created by Devin Mooers on 1/12/14.
//
//

#ifndef __Synthesis__MIDIReceiver__
#define __Synthesis__MIDIReceiver__

#include <iostream>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-tokens"
#include "IPlug_include_in_plug_hdr.h"
#pragma clang diagnostic pop

#include "IMidiQueue.h"
#include "Signal.h"
using Gallant::Signal1;
using Gallant::Signal2; //Signal2 is a signal that passes two parameters. There's Signal0 through Signal8, so you can choose depending on how many parameters you need.

class MIDIReceiver {
    
public:
    MIDIReceiver() :
    mNumKeys(0),
    mOffset(0) {
        for (int i = 0; i < keyCount; i++) {
            mKeyStatus[i] = false;
        }
    };
    
    // Returns true if the key with a given index is currently pressed
    inline bool getKeyStatus(int keyIndex) const { return mKeyStatus[keyIndex]; }
    // Returns the number of keys currently pressed
    inline int getNumKeys() const { return mNumKeys; }
    void advance();
    void onMessageReceived(IMidiMsg* midiMessage);
    inline void Flush(int nFrames) { mMidiQueue.Flush(nFrames); mOffset = 0; }
    inline void Resize(int blockSize) { mMidiQueue.Resize(blockSize); }
    
    // both of these signal generators will pass two ints
    Signal2< int, int > noteOn;
    Signal2< int, int > noteOff;
    Signal1< double > sustainChange;
    Signal1< double > expressionChange;
    Signal1< double > modChange;
    
private:
    IMidiQueue mMidiQueue;
    static const int keyCount = 128;
    int mNumKeys; // how mnay keys are being played at the moment (via midi)
    bool mKeyStatus[keyCount]; // array of on/off for each key (index is note number)
    int mOffset;
};

#endif /* defined(__Synthesis__MIDIReceiver__) */
