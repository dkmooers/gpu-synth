#include "Synthesis.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmain"
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
#include "IPlug_include_in_plug_src.h"
#pragma clang diagnostic pop
#include "IControl.h"
#include "IPlugMultiTargets_controls.h"
#include "IKeyboardControl.h"
#include "resource.h"

const int kNumPrograms = 5; // number of presets to include (will fill with "Empty" if not enough presets are declared below)
const double parameterStep = 0.001;
enum EParams
{
  mB,
  mNumPartials,
  mStringDetuneRange,
  mPartialDetuneRange,
  mDamping,
  mLinearTerm,
  mSquaredTerm,
  mCubicTerm,
  mBrightnessA,
  mBrightnessB,
  mPitchBendCoarse,
  mPitchBendFine,
  //mNoisyTransient,
  kNumParams
};

typedef struct {
  const char* name;
  const int x1;
  const int y1;
  const int x2;
  const int y2;
  const double defaultVal;
  const double minVal;
  const double maxVal;
  const double step;
  /// also need a shape parameter in here!
} parameterProperties_struct;


const parameterProperties_struct parameterProperties[kNumParams] =
{
  {
    .name="Inharmonicity",
    .x1 = 50,
    .y1 = 100,
    .x2 = 50+60,
    .y2 = 100+90,
    .defaultVal = 0.003,
    .minVal = 0.0,
    .maxVal = 1.0,
    .step = 0.0001
  },
  {
    .name="Partials",
    .x1 = 150,
    .y1 = 50,
    .x2 = 150+60,
    .y2 = 50+90
  },
  {
    .name="String Detune Range",
    .x1 = 250,
    .y1 = 100,
    .x2 = 250+60,
    .y2 = 100+90,
    .defaultVal = 0.001,
    .minVal = 0.0001,
    .maxVal = 0.01,
    .step = 0.0001
  },
  {
    .name="Partial Detune Range",
    .x1 = 350,
    .y1 = 50,
    .x2 = 350+60,
    .y2 = 50+90,
    .defaultVal = 1.0,
    .minVal = 0.1,
    .maxVal = 2.0,
    .step = 0.001
  },
  {
    .name="Damping",
    .x1 = 450,
    .y1 = 100,
    .x2 = 450+60,
    .y2 = 100+90,
    .defaultVal = 2.5,
    .minVal = 0.1,
    .maxVal = 10.0,
    .step = 0.001
  },
  {
    .name="Linear Term",
    .x1 = 50,
    .y1 = 250,
    .x2 = 50+60,
    .y2 = 250+90,
    .defaultVal = 0.3,
    .minVal = 0.001,
    .maxVal = 1.0,
    .step = 0.001
  },
  {
    .name="Squared Term",
    .x1 = 150,
    .y1 = 200,
    .x2 = 150+60,
    .y2 = 200+90,
    .defaultVal = 1.0,
    .minVal = 0.001,
    .maxVal = 1.0,
    .step = 0.001
  },
  {
    .name="Cubic Term",
    .x1 = 250,
    .y1 = 250,
    .x2 = 250+60,
    .y2 = 250+90,
    .defaultVal = 0.3,
    .minVal = 0.001,
    .maxVal = 1.0,
    .step = 0.001
  },
  {
    .name="Brightness A",
    .x1 = 350,
    .y1 = 200,
    .x2 = 350+60,
    .y2 = 200+90,
    .defaultVal = 0.4,
    .minVal = 0.001,
    .maxVal = 1.0,
    .step = 0.001
  },
  {
    .name="Brightness B",
    .x1 = 450,
    .y1 = 250,
    .x2 = 450+60,
    .y2 = 250+90,
    .defaultVal = 0.2,
    .minVal = 0.001,
    .maxVal = 1.0,
    .step = 0.001
  },
  {
    .name="Pitch Bend Coarse",
    .x1 = 50,
    .y1 = 350,
    .x2 = 50+60,
    .y2 = 350+90,
    .defaultVal = 0.5,
    .minVal = 0.001,
    .maxVal = 1.0,
    .step = 0.001
  },
  {
    .name="Pitch Bend Fine",
    .x1 = 150,
    .y1 = 350,
    .x2 = 150+60,
    .y2 = 350+90,
    .defaultVal = 0.5,
    .minVal = 0.001,
    .maxVal = 1.0,
    .step = 0.001
  }
//  {
//    .name="Noisy Transient",
//    .x1 = 50,
//    .y1 = 374,
//    .x2 = 50+60,
//    .y2 = 374+90
//  }
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,
  kKeybX = 1,
  kKeybY = 720

  //kFrequencyX = 79,
  //kFrequencyY = 62,
  //kKnobFrames = 128
};

Synthesis::Synthesis(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo),
  lastVirtualKeyboardNoteNumber(virtualKeyboardMinimumNoteNumber - 1)
{
  TRACE;
  
  CreateParams();
  CreateGraphics();
  CreatePresets();
  
  VoiceManager& voiceManager = VoiceManager::getInstance();
  voiceManager.initOpenCL();
  
  //openCLStarted = false;
  
  // connet the noteOn's and noteOff's from mMIDIReceiver to VoiceManager using signals and slots
  mMIDIReceiver.noteOn.Connect(&voiceManager, &VoiceManager::onNoteOn);
  mMIDIReceiver.noteOff.Connect(&voiceManager, &VoiceManager::onNoteOff);
  mMIDIReceiver.sustainChange.Connect(&voiceManager, &VoiceManager::onSustainChange);
  mMIDIReceiver.expressionChange.Connect(&voiceManager, &VoiceManager::onExpressionChange);
  mMIDIReceiver.modChange.Connect(&voiceManager, &VoiceManager::onModChange);
  //mMIDIReceiver.noteOn.Connect(mOscilloscope, &Oscilloscope::updatePixelColors);
}

Synthesis::~Synthesis() {}

void Synthesis::CreateParams() {
  for (int i = 0; i < kNumParams; i++) {
    IParam* param = GetParam(i);
    const parameterProperties_struct& properties = parameterProperties[i];
    switch (i) {
      // Int parameters:
      case mNumPartials:
        param->InitInt(properties.name,
                        10, // default
                        1, // min
                        200); // max
        break;
      // Bool parameters:
//      case mNoisyTransient:
//        param->InitBool(properties.name, true);
//        break;
      // Double parameters:
      default:
        param->InitDouble(properties.name,
                          properties.defaultVal,
                          properties.minVal,
                          properties.maxVal,
                          parameterStep);
        break;
    }
  }
  
  /// set shape for knobs
  GetParam(mB)->SetShape(5);
  GetParam(mNumPartials)->SetShape(1);
  
  /// initialize correct default parameter values on load
  for (int i = 0; i < kNumParams; i++) {
    OnParamChange(i);
  }
}

void Synthesis::CreateGraphics() {
  
  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachPanelBackground(&COLOR_BLACK);
  //pGraphics->AttachBackground(BG_ID, BG_FN);
  
  IBitmap whiteKeyImage = pGraphics->LoadIBitmap(WHITE_KEY_ID, WHITE_KEY_FN, 6);
  IBitmap blackKeyImage = pGraphics->LoadIBitmap(BLACK_KEY_ID, BLACK_KEY_FN);
  
  //mOscilloscope = new Oscilloscope(this, IRECT(0, 300, GUI_WIDTH, GUI_HEIGHT));
  
  //                            C#      D#          F#      G#      A#
  int keyCoordinates[12] = { 0, 10, 17, 30, 35, 52, 61, 68, 79, 85, 97, 102 };

  mVirtualKeyboard = new IKeyboardControl(this, kKeybX, kKeybY, virtualKeyboardMinimumNoteNumber, /* octaves: */ 8, &whiteKeyImage, &blackKeyImage, keyCoordinates);
  
  pGraphics->AttachControl(mVirtualKeyboard);
  
  // Knob bitmap
  IBitmap knobBitmap = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, 101);
  
  for (int i = 0; i < kNumParams; i++) {
    const parameterProperties_struct& properties = parameterProperties[i];
    IControl* control;
    IControl* labelcontrol;
    IBitmap* graphic;
    IText text = IText(14, &COLOR_WHITE);
    IText labeltext = IText(14, &COLOR_WHITE);

    switch (i) {
        // Knobs:
      default:
        graphic = &knobBitmap;
        control = new IKnobMultiControlText(this, IRECT(properties.x1, properties.y1, properties.x2, properties.y2), i, graphic, &text);
        labelcontrol = new ITextControl(this, IRECT(properties.x1, properties.y1-20, properties.x2, properties.y1), &labeltext, properties.name);
        break;
    }
    pGraphics->AttachControl(control);
    pGraphics->AttachControl(labelcontrol);
  }
  
  
  
//  // caption
//  IText mBLabel(14, &COLOR_WHITE);
//  pGraphics->AttachControl(new ITextControl(this, IRECT(60-12, 154-20, 60+50, 154+20), &mBLabel, "Inharmonicity"));
//  
  
  
//  // TEXT TESTING
//  
//  IRECT tmpRect4(650, 50, 800, 120);
//  IText textProps4(40, &COLOR_WHITE, "Helvetica", IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
//  pGraphics->AttachControl(new ITextControl(this, tmpRect4, &textProps4, "STRING SYNTH"));
  
  
  
  //pGraphics->AttachControl(mOscilloscope);
  
  AttachGraphics(pGraphics);
}


///////////////////////////---------////////////////////////////

void Synthesis::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.
  
  double *leftOutput = outputs[0];
  double *rightOutput = outputs[1];
  VoiceManager& voiceManager = VoiceManager::getInstance();
  processVirtualKeyboard();
  
  
  
  
  //std::cout << "\nBlock size = " << GetBlockSize();

  
//  blockOfSamples = voiceManager.getBlockOfSamples();
//  for (int i = 0; i < BLOCK_SIZE; i++) {
//    leftOutput[i] = blockOfSamples[i*NUM_CHANNELS];
//    rightOutput[i] = blockOfSamples[i*NUM_CHANNELS + 1];
////    if (i < 512) {
////      printf("\ni = %d, left = %f, right = %f", i, leftOutput[i], rightOutput[i]);
////    }
//    //printf("\nleftOutput index = %d, rightOutput index = %d", i*NUM_CHANNELS, i*NUM_CHANNELS+1);
//    mOscilloscope->updateLastSample(leftOutput[i], rightOutput[i]);
//    mMIDIReceiver.advance();
//  }
  
  //std::thread voiceEnergyUpdater(&Synthesis::voiceEnergyUpdaterManager, this);
  
  
  /// this allows for BLOCK_SIZE to be different (smaller) than actual VST host blockSize, to reduce latency
  int numBlocks = nFrames/BLOCK_SIZE;
  
  
  for (int j = 0; j < numBlocks; j++) {
    
    blockOfSamples = voiceManager.getBlockOfSamples();
    for (int i = 0; i < BLOCK_SIZE; i++) {
      
      leftOutput[i + j*BLOCK_SIZE] = clip(blockOfSamples[i*NUM_CHANNELS]);
      rightOutput[i + j*BLOCK_SIZE] = clip(blockOfSamples[i*NUM_CHANNELS + 1]);
      
      // add in bandlimited noise
//      leftOutput[i + j*BLOCK_SIZE] += 0.003f * (static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/2.0f)) - 1.0f) * blockOfSamples[i*NUM_CHANNELS];
//      rightOutput[i + j*BLOCK_SIZE] += 0.003f * (static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/2.0f)) - 1.0f) * blockOfSamples[i*NUM_CHANNELS + 1];
      
      
//      mOscilloscope->updateLastSample(leftOutput[i], rightOutput[i]);
      mMIDIReceiver.advance();
      voiceManager.updateVoiceDampingAndEnergy(i);
    }
    mMIDIReceiver.Flush(nFrames/numBlocks);
    voiceManager.setFreeInaudibleVoices();
  }
  
  
  
  
//  for (int i = 0; i < nFrames/BLOCK_SIZE; i++) {
//    blockOfSamples = voiceManager.getBlockOfSamples();
//    for (int j = 0; j < BLOCK_SIZE; j++) {
//      leftOutput[i*BLOCK_SIZE + j] = blockOfSamples[i*NUM_CHANNELS];
//      rightOutput[i*BLOCK_SIZE + j] = blockOfSamples[i*NUM_CHANNELS + 1];
//      mOscilloscope->updateLastSample(leftOutput[i], rightOutput[i]);
//      mMIDIReceiver.advance();
//    }
//  }
  
  //mMIDIReceiver.Flush(nFrames);
}

double Synthesis::clip(double n) {
  return std::max(-0.99, std::min(n, 0.99));
}


///////////////////////////---------////////////////////////////





void Synthesis::Reset()
{
  TRACE;
  IMutexLock lock(this);
  double sampleRate = GetSampleRate();
  VoiceManager::getInstance().setSampleRate(sampleRate);
}

void Synthesis::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);
  VoiceManager& voiceManager = VoiceManager::getInstance();
  IParam* param = GetParam(paramIdx);
//  std::cout << paramIdx << "\n";
//  printf("\nparam: %s", paramIdx);
  switch(paramIdx) {
      // Volume Envelope:
    case mB:
      voiceManager.updateInharmonicityCoeff(param->Value());
      break;
    case mNumPartials:
      voiceManager.updateNumPartials(param->Value());
      break;
    case mStringDetuneRange:
      voiceManager.updateStringDetuneRange(param->Value());
      break;
    case mPartialDetuneRange:
      voiceManager.updatePartialDetuneRange(param->Value());
      break;
    case mDamping:
      voiceManager.updateDamping(param->Value());
      break;
    case mLinearTerm:
      voiceManager.updateLinearTerm(param->Value());
      break;
    case mSquaredTerm:
      voiceManager.updateSquaredTerm(param->Value());
      break;
    case mCubicTerm:
      voiceManager.updateCubicTerm(param->Value());
      break;
    case mBrightnessA:
      voiceManager.updateBrightnessA(param->Value());
      break;
    case mBrightnessB:
      voiceManager.updateBrightnessB(param->Value());
      break;
    case mPitchBendCoarse:
      voiceManager.updatePitchBendCoarse(param->Value());
      break;
    case mPitchBendFine:
      voiceManager.updatePitchBendFine(param->Value());
      break;
    default:
      break;
  }
}

// This function will be called whenever the application receives a MIDI message. We're passing the messages through to our MIDI receiver.
void Synthesis::ProcessMidiMsg(IMidiMsg* pMsg) {
  mMIDIReceiver.onMessageReceived(pMsg);
  mVirtualKeyboard->SetDirty();
}

void Synthesis::processVirtualKeyboard() {
  IKeyboardControl* virtualKeyboard = (IKeyboardControl*) mVirtualKeyboard;
  int virtualKeyboardNoteNumber = virtualKeyboard->GetKey() + virtualKeyboardMinimumNoteNumber;
  
  if(lastVirtualKeyboardNoteNumber >= virtualKeyboardMinimumNoteNumber && virtualKeyboardNoteNumber != lastVirtualKeyboardNoteNumber) {
    // The note number has changed from a valid key to something else (valid key or nothing). Release the valid key:
    IMidiMsg midiMessage;
    midiMessage.MakeNoteOffMsg(lastVirtualKeyboardNoteNumber, 0);
    mMIDIReceiver.onMessageReceived(&midiMessage);
  }
  
  if (virtualKeyboardNoteNumber >= virtualKeyboardMinimumNoteNumber && virtualKeyboardNoteNumber != lastVirtualKeyboardNoteNumber) {
    // A valid key is pressed that wasn't pressed the previous call. Send a "note on" message to the MIDI receiver:
    IMidiMsg midiMessage;
    midiMessage.MakeNoteOnMsg(virtualKeyboardNoteNumber, virtualKeyboard->GetVelocity(), 0);
    mMIDIReceiver.onMessageReceived(&midiMessage);
  }
  
  lastVirtualKeyboardNoteNumber = virtualKeyboardNoteNumber;
}

void Synthesis::CreatePresets() {
}