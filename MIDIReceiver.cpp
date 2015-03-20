//
//  MIDIReceiver.cpp
//  Synthesis
//
//  Created by Devin Mooers on 1/12/14.
//
//

#include "MIDIReceiver.h"

void MIDIReceiver::onMessageReceived(IMidiMsg* midiMessage) {
    IMidiMsg::EStatusMsg status = midiMessage->StatusMsg();
    IMidiMsg::EControlChangeMsg controlChange = midiMessage->ControlChangeIdx();
    // We're only interested in Note On/Off messages (not CC, pitch, etc.)
    if(status == IMidiMsg::kNoteOn || status == IMidiMsg::kNoteOff) {
        //mMidiQueue.Add(midiMessage);
    }
    if(status == IMidiMsg::kPitchWheel) {
//        printf("\nPITCH WHEEL! %f", midiMessage->PitchWheel());
    }
    if(controlChange == IMidiMsg::kSustainOnOff) {
//        printf("\nSUSTAYN %f", midiMessage->ControlChange(controlChange));
    }
    if(controlChange == IMidiMsg::kModWheel) {
//        printf("\nMOD WHEEEEL %f", midiMessage->ControlChange(controlChange));
    }
    if(controlChange == IMidiMsg::kExpressionController) {
//        printf("\nEXPRESSISIIIOOON %f", midiMessage->ControlChange(controlChange));
    }
    mMidiQueue.Add(midiMessage);
}

// this is called on every sample while we're generating a buffer; as long as there are messages in the queue, we're processing and removing them from the front (using Peek and Remove). But we only do this for MIDI messages whose mOffset isn't greater than the current offset into the buffer. This means that we process every message at the right sample, keeping the relative timing intact.
// After reading noteNumber and velocity, the if statement distinguishes note on and off messages (no velocity is interpreted as note off). In both cases, we're keeping track of which notes are being played, as well as how many of them. We also update the mLast... members so the already mentioned last note priority happens. This is the place where we know the frequency has to change, so we're updating it here. Finally, the mOffset is incremented so that the receiver knows how far into the buffer it currently is. An alternative would be to pass the offset in as an argument. So we now have a class that can receive all incoming MIDI note on/off messages. It always keeps track of which notes are being played and what the last played note (and frequency) was. 
void MIDIReceiver::advance() {
    while (!mMidiQueue.Empty()) {
        IMidiMsg* midiMessage = mMidiQueue.Peek();
        if (midiMessage->mOffset > mOffset) break;
        
        IMidiMsg::EStatusMsg status = midiMessage->StatusMsg();
        int noteNumber = midiMessage->NoteNumber();
        int velocity = midiMessage->Velocity();
        
        // There are only note on/off messages in the queue, see ::OnMessageReceived
        if (status == IMidiMsg::kNoteOn && velocity) {
            if(mKeyStatus[noteNumber] == false) {
                mKeyStatus[noteNumber] = true;
                mNumKeys += 1;
                noteOn(noteNumber, velocity);
            }
        } else {
            if(mKeyStatus[noteNumber] == true) {
                mKeyStatus[noteNumber] = false;
                mNumKeys -= 1;
                noteOff(noteNumber, velocity);
            }
        }
        
        IMidiMsg::EControlChangeMsg controlChange = midiMessage->ControlChangeIdx();
        switch (controlChange) {
            case IMidiMsg::kSustainOnOff:
                sustainChange(midiMessage->ControlChange(controlChange));
                break;
            case IMidiMsg::kModWheel:
                modChange(midiMessage->ControlChange(controlChange));
                break;
            case IMidiMsg::kExpressionController:
                expressionChange(midiMessage->ControlChange(controlChange));
                break;
            default:
                break;
        }
        
        mMidiQueue.Remove();
    }
    mOffset++;
}