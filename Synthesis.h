#ifndef __SYNTHESIS__
#define __SYNTHESIS__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-tokens"
#include "IPlug_include_in_plug_hdr.h"
#pragma clang diagnostic pop
#include "MIDIReceiver.h"
#include "Oscilloscope.h"
#include "VoiceManager.h"
#include "Filter.h"
#include <iostream>
#include <boost/array.hpp>

class Synthesis : public IPlug
{
public:
  
  Synthesis(IPlugInstanceInfo instanceInfo);
  ~Synthesis();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  void ProcessMidiMsg(IMidiMsg* pMsg); // to receive MIDI messages
  
  inline int GetNumKeys() const { return mMIDIReceiver.getNumKeys(); }; // Needed for the GUI keyboard - should return non-zero if one or more keys are playing.
  inline bool GetKeyStatus(int key) const { return mMIDIReceiver.getKeyStatus(key); }; // Should return true if the specified key is playing
  static const int virtualKeyboardMinimumNoteNumber = 24;
  int lastVirtualKeyboardNoteNumber;
  boost::array<double, 2> nextSample;

private:
  
  void CreateParams();
  void CreateGraphics();
  void CreatePresets();
  void InitOpenCL();
  MIDIReceiver mMIDIReceiver;
  IControl* mVirtualKeyboard;
  void processVirtualKeyboard();
  double clip(double sample);
//  Oscilloscope* mOscilloscope;
  OpenCL mOpenCL;
  Filter mFilterL;
  Filter mFilterR;
  boost::array<double, BLOCK_SIZE*NUM_CHANNELS> blockOfSamples;
  
  void voiceEnergyUpdaterManager();

};

#endif
