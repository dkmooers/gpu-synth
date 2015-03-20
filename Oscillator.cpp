//
//  Oscillator.cpp
//  Synthesis
//
//  Created by Devin Mooers on 1/12/14.
//
//

#include "Oscillator.h"
//#include <vector>
#include <boost/array.hpp>
#include <OpenCL/OpenCL.h>

//#include <limits>
//#include <cmath>
//double epsilon = std::numeric_limits<double>::denorm_min();

double Oscillator::mSampleRate = 44100.0;

void Oscillator::setMode(OscillatorMode mode) {
    mOscillatorMode = mode;
}

void Oscillator::setFrequency(double frequency) {
    mFrequency = frequency;
    if (mFrequency != mLastFrequency) {
        mLastFrequency = mFrequency;
        updateInstrumentModel(); // update instrument model only when new frequency is different than last calculated/stored one /// ERROR - this means it won't re-run if you hit the same note twice in a row! figure out a better method... call from onNoteOn?
    }
}

void Oscillator::setVelocity(int velocity) {
    mVelocity = velocity;
}

void Oscillator::setSampleRate(double sampleRate) {
    mSampleRate = sampleRate;
    mTimeStep = 1.0 / sampleRate;
    updateInstrumentModel();
}

void Oscillator::updateTime() {
    mTime = 0.0; // reset amplitude increment on every new note trigger (this is used to calculate accelerating decays for higher partials)
}

// only need to update numpartials and string frequencies with every noteOn, I think (or pitch bend, actually), but DEFINITELY not needed with every stinkin sample
void Oscillator::updateInstrumentModel() {
    
    updateTime();
    
    /// update frequency based on pitch mod
    
    double pitchModAsFrequency = pow(2.0, fabs(mPitchMod) * 14.0) - 1;
    if (mPitchMod < 0) {
        pitchModAsFrequency = -pitchModAsFrequency;
    }
    double calculatedFrequency = fmin(fmax(mFrequency + pitchModAsFrequency, 0), mSampleRate / 2.0); // ensures that f is greater than 0 and less than nyquist
    
    mFrequency = calculatedFrequency;
    
    /// update string ratio
    float LO = 1.0 - mStringDetuneRange;
    float HI = 1.0 + mStringDetuneRange;
    mStringRatio = LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));
    
    /// update chaotic attractor values per note hit
    //mLRand = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 0.5 + 0.5; // 0.5 to 1.0
    
    /// update random partial frequency multipliers and amplitudes
    float randAmpScalar = 0.0;
    float freqOffsetRange = 0.002;
    float freqLO = 1.0 - freqOffsetRange;
    float freqHI = 1.0 + freqOffsetRange;
    float ampOffsetRange = 0.0;
    float ampLO = 1.0 - ampOffsetRange;
    float ampHI = 1.0 + ampOffsetRange;
    
    double partialInharmonicityCoeffTimesFreq;
    
    for (int i = 1; i < mNumPartials + 1; i++) {
        
        /// OPTIMIZATION: actually just calculate an array of these whenever B changes - one for each partial index up to max number of partials. That way I don't have to calc a bunch of sqrt's every time a new note is hit - only when B is changed.
        partialInharmonicityCoeffTimesFreq = mFrequency * sqrt((1 + mB * i * i)); // only need to calc once per approx partial freq
        
        /// calculate first string partial frequencies
        mPartialFrequencies[(i-1)*2] = partialInharmonicityCoeffTimesFreq * i * (freqLO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(freqHI-freqLO)))); // Harvey Fletcher's equation for partial inharmonicity, just a multiplier here (for multiplying actual partial frequency);
        
        /// calculate second string partial frequencies
        mPartialFrequencies[(i-1)*2+1] = partialInharmonicityCoeffTimesFreq * i * (freqLO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(freqHI-freqLO)))) * mStringRatio;
        
        /// calculate first string random partial amplitudes
        randAmpScalar = (ampLO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(ampHI-ampLO))));
        mPartialAmplitudes[(i-1)*2] = randAmpScalar; // assign random partial amplitude multiplier to each
        
        /// calculate second string random partial amplitudes
        randAmpScalar = (ampLO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(ampHI-ampLO))));
        mPartialAmplitudes[(i-1)*2+1] = randAmpScalar; // assign random partial amplitude multiplier to each
        
        /// calculate first string partial pan values
        mPartialPanValues[(i - 1)*2] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX); // 0 to 1
        
        /// calculate second string partial pan values
        mPartialPanValues[(i - 1)*2+1] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX); // 0 to 1

    }
}

void Oscillator::updateInharmonicityCoeff(double mBNew) {
    mB = mBNew;
    //std::cout << "\nmB = " << mB;
    updateInstrumentModel();
}

void Oscillator::updateNumPartials(int mNumPartialsNew) {
    mNumPartials = mNumPartialsNew;
    //std::cout << "\nmNumPartials = " << mNumPartials;
    updateInstrumentModel();
}

void Oscillator::setPitchMod(double amount) {
    mPitchMod = amount;
    updateInstrumentModel();
}

void Oscillator::getLocationBasedAmplitude(int i) {
    // takes partial index (harmonic index) and string strike location and returns amplitude of harmonic
    
    mStringHitLocation = 0.2 - mVelocity / 635.0; // range between 0 and 0.2 - the higher the velocity, the closer it is toward the bridge! (makes for brighter sound)

    mLocationBasedAmplitude = sin(mPI*mStringHitLocation*i); // string strike location = x, wavelength gamma (relative to string length) = 2 / i, and amplitude = sin(2*pi*x / gamma)
    
    // edit this to instead be an array of location-based amplitudes that gets updated with every note trigger (### or add it to mpartialamplitudemultipliers calculation above!!!)
}




boost::array<double, 2> Oscillator::nextSample() {
    
    samples[0] = samples[1] = 0.0; // reset samples to zeroes
    
    switch (mOscillatorMode) {
            
            
            
        /// Make an OpenCL kernel built into Oscillator class... :D (copy "square" kernel from hello_world example and make it sin)
        
        /// make mTransientLength into a knob! (so I can select on the fly)
        /// make mStringDetuneRange into a knob!
        /// make mAttack into a knob! ranging from 1000 to 1,000,000 perhaps.
        /// implement an on/off switch for transient noise! (and eventually a selector for which type of transient noise - lorenz? tinkerbell? etc., along with parameters)
        /// make a gain knob!
            
        
        /// implement mB scaling, so lower notes get a lower mB, and higher notes get higher mB (so high notes sound piano-like, but low notes don't sound disgustingly out of tune)

        /// Synthesis.cpp: improve the keyboard graphic using spacing here (gotta make BG graphic, and uplaod new key images) http://www.martin-finke.de/blog/articles/audio-plugins-015-redesign/
            
            

        /// Start reading about OpenCL... how to code with OpenCL in C++? Include a framework? Then how to do audio processing on it? Can I do it per-sample?
            
        
        /// look up more efficient sine oscillator methods here: http://www.musicdsp.org/archive.php?classid=1#241 // http://www.musicdsp.org/archive.php?classid=1
        // or here: https://ccrma.stanford.edu/~jos/pasp04/Digital_Waveguide_Oscillator.html and here for freq masking improvements: http://www.csis.ul.ie/dafx01/proceedings/papers/lagrange.pdf
        
        /// OPTIMIZATION: in updateInstrumentModel, actually just calculate an array of inharmonicity coefficients whenever B changes - one for each partial index up to max number of partials. That way I don't have to calc a bunch of sqrt's every time a new note is hit - only when B is changed.
            
 
        /// try adding in fastsin() again (taylor series) to see if it runs faster now that I'm calculating more partials!

        /// PERFORMANCE: try simplifying the partial amplitude calculations - the 2500-8000Hz one has THIRTEEN MULTIPLIES/DIVIDES! That's ridiculous! Doesn't need to be that complex... could be WAY simpler. Could strip to half that many or LESS multiplies. Same with < 2500Hz, maybe the others.
    
        /// NEXT: implement pitch mod! http://www.martin-finke.de/blog/articles/audio-plugins-017-polyphony-ii/
            
        /// try solving differential equations instead of calculating sines - calculate TWO differential equations, one for each string! that should be complex enough, shouldn't it? especially if i run it through the dulcimer impulse response afterward? (wouldn't be surprised if that's how PianoTeq works)
        
        /// research TRANSIENTS... how to synthesize different types of transients that sound really good / rich / fuzzy / plucky / fingerpicking / pick-picking / etc.? (maybe get an audio sample of a picked mandolin string and inspect the waveform... also could record a dulcimer hammer hit (damped with no string sound, just transient attack) and inspect THAT waveform for clues... and do reassigned spectrogram on it for an even HIGHER resolution window into the sound!)
            
        /// extract an even LONGER dulcimer impulse response for even higher quality sound!
        
        /// NEXT: FIGURE OUT THE DAMN CLICK. whenever we trigger a new note, we need to WAIT before going to the attack stage, before resetting time counter, etc. - we need to DELAY this by ~1ms and fade out the existing sound, otherwise we'll get a major click. The only alternative to this would be by implementing polyphony, then I won't need this delay/fadeout. But seeing as this already kills my computer with one voice... try eliminating click first. (Or try faster sine calculations first, and see if I can get more like 500 partials going at once.)
            
        
        /// LATER:
            
            /// Implement a second oscillator, that's actually just the second string (maybe). That way, you can indepdently change mB, numpartials, resonant body impulse response, etc. for each voice. (I can do this later on. Follow tutorial here: http://www.martin-finke.de/blog/articles/audio-plugins-017-polyphony-ii/ )
            
            /// implement frequency masking, i.e. stop calculating inaudible partials, esp. important in polyphony http://www.csis.ul.ie/dafx01/proceedings/papers/lagrange.pdf
            

            
        // implement some more attractors from Evernote to see what they sound like (better noisy transients? harmonic material?)
            
        // do performance profiling in Xcode, with both "Analyze" and "Instruments" (and/or use "gprof", which should show me running time of different functions so I can really drill down and find the worst ones)
            
        // improve code using BOOST (read about "boost performance enhacements" or something)
            

            
            
        ///////////////////////////---------////////////////////////////
            
            
        case OSCILLATOR_MODE_SINE:
            
            if (mTime > mPartialsDelay) { // delay partials slightly so transient/attack/noise happens slightly earlier
                
                for (int j = 1; j < mNumPartials + 1; j++) { // start on first partial
                    
                    mPartialFrequency = mPartialFrequencies[(j-1)*2]; // only approx, for calculating amplitude envelopes, only using the first string partial freq, not the second, for efficiency
   
                    // I should really calculate a running amplitude here for each partial... so that when partials get below the audible threshold, just stop rendering them (for performance)??? or just keep using the per-sample "break" method below
                    
                    /// calculate partial amplitude
                    
                    if (mPartialFrequency < 2500) {
                        //samples += sin(mPhase * (j + 1)) / ((j + 1) * 127.0 * mTime / mVelocity + 1); // increasing partials, where mPhase * j is proportional to partial frequency; amplitude scales down as 1/x with increasing partials; also, sound gets darker w/lower velocity (b/c higher partials get scaled down in amplitude faster the lower the velocity is)
                        // mTime increases as time goes on, and used here, it makes higher partials decay faster. use the equation "y = 1 / (x * (127 * incr / velocity) + 1)" in Grapher to test this out
                        mAmplitude = 4.0/(j+1)/(mTime+0.5/(j+1))*(1.0/(sqrt(j+1)))*(1.0/(j+1)) * (1.0-(1-mVelocity/127.0)*mPartialFrequency/2500);
                    } else if (mPartialFrequency < 8000) {
                        if (mAmplitude < 0) {
                            mAmplitude = 0;
                        } else {
                            mAmplitude = 2.0/(j+1)/(mTime+0.5/(j+1))*(1.0/((j+1))*(1.0/(j+1))) * (1.0-mTime*mPartialFrequency/5000) * (1.0-(1-mVelocity/127.0)*mPartialFrequency/8000);
                        }
                    } else if (mPartialFrequency < 20000) {
                        mAmplitude = 1/((pow((j+1), 3))*(mTime+0.0001)) - 0.00001;
                        if (mAmplitude < 0) {
                            mAmplitude = 0;
                        } else {
                            mAmplitude *= (1.0-mTime*mPartialFrequency/3000); // linear decay term
                        }
                    } else {
                        mAmplitude = 0;
                        break;
                    }
                    
                    if (mAmplitude <= 0) {
                        break;
                    }
                    
//                    if ( std::abs(mAmplitude) < epsilon ) {
//                        //log_debug("detected small num, %Le, %Le", x, y);
//                        std::cout << "\nDenormal detected";
//                    }
                    
                    /// envelope follower:
                    // for first partial (Fundamental frequency), i.e. if j=1, just store the current mAmplitude in a member variable called mAmplitudeFollower or mCurrentEnvelope or mEnvelopeValue. Then, when voice stealing, find the voice with the lowest mEnvelopeValue and steal THAT one (since it'll be the quietest).
                    
                    // adjust partial amplitude based on where the string was struck
                    /// edit: I should really just store an array of these, and calculate it once on every string hit, rather than once with every sample, right?!
                    
                    //getLocationBasedAmplitude(j);
                    //mAmplitude *= mLocationBasedAmplitude;
                    
                    // adjust partial "brightness" based on velocity and partial frequency (higher partials get muted at lower velocities, and no effect on any partials at high velocity)
                    /// performance boost: calculate an array of these brightness amplitudes only on each string hit (build them in to amplitude calculation that already happens in updateInstrumentModel() ) --> improve it, too, not doing much currently
                    //mAmplitude *= (1 - (1 - mVelocity/127.0) * mPartialFrequency/mMaxFreq);
                    
                    
                    
                    /// calculate this partial for this sample (one for each of two strings)
                    
                    //double twoPITimesTime = twoPI * mTime;
                    
                    //values[0] = values[1] = 0;
                    
                    values[0] = (sin(twoPI * (mTime - mPartialsDelay) * mPartialFrequencies[((j-1)*2)]) * mPartialAmplitudes[((j-1)*2)]) * mAmplitude;
                    values[1] = (sin(twoPI * (mTime - mPartialsDelay) * mPartialFrequencies[((j-1)*2+1)]) * mPartialAmplitudes[((j-1)*2+1)]) * mAmplitude;

                    
                    /// non-reverb wash (just random panning)
                    
                    // set right channel amplitude using left channel samples
                    //samples[0] += (values[0] * (1 - mPartialPanValues[((j-1)*2)]) + values[1] * (1-mPartialPanValues[((j-1)*2+1)])) * 0.1;
                    // set left channel amplitude
                    //samples[1] += (values[0] * mPartialPanValues[((j-1)*2)] + values[1] * mPartialPanValues[((j-1)*2+1)]) * 0.1;
                    
                    
                    
                    /// reverb wash (sound starts at center, then widens L/R over time)
                    
                    double panLeft1 = 1 - mPartialPanValues[((j-1)*2)];
                    double panLeft2 = 1 - mPartialPanValues[((j-1)*2+1)];
                    double panRight1 = mPartialPanValues[((j-1)*2)];
                    double panRight2 = mPartialPanValues[((j-1)*2+1)];
                    double panMult = 5;
                    
                    samples[0] += (values[0] * (panLeft1 - (panLeft1 - 0.5)/(panMult * mTime + 1)) + values[1] * (panLeft2 - (panLeft2 - 0.5)/(panMult * mTime + 1))) * 0.03;
                    samples[1] += (values[0] * (panRight1 - (panRight1 - 0.5)/(panMult * mTime + 1)) + values[1] * (panRight2 - (panRight2 - 0.5)/(panMult * mTime + 1))) * 0.03;
                    
                    
                    if (fabs(samples[0]) >= 1 && fabs(values[0]) >= 1) {
                        std::cout << "\n CLIP -- value = " << values[0] << ", sample = " << samples[0];
                    }
                    
                    
//                    if ( std::abs(values[0]) < epsilon ) {
//                        //log_debug("detected small num, %Le, %Le", x, y);
//                        std::cout << "\nDenormal detected";
//                    }
                    
                    
                    //samples[1] += (values[0] * mPartialPanValues[((j-1)*2)] + values[1] * mPartialPanValues[((j-1)*2+1)]) * 0.1;
                    
                    //std::cout << "pan value (string1) = " << mPartialPanValues[((j-1)*2)] << std::endl;
                    //std::cout << "pan value (string2) = " << mPartialPanValues[((j-1)*2+1)] << std::endl;
                    
                }
            }
            
            
            
            //std::cout << "freq of string 1 = " << mPhase * mSampleRate / twoPI / (mTime/mTimeStep) << std::endl;
            

            // ADDING TRANSIENTS
            
            /// exponential decay transient (lorenz)
            
            
            /// LATER: make Rho into a knob, so you can change the transient flavor (range somewhere 24-44 ish, explore this)
            
            if (mCalculateNoisyTransient) {
                if (mTime < mTransientLength) { // only calculate for the initial transient attack, then stop doing attractor updates (may want to remove this behavior later, for doing long-lasting noise / radical sound design)
                    
                    
                    //                // Lorenz attractor:
                    //
                    //                mLXdot = mLSigma * (mLY - mLX);
                    //                mLYdot = mLX * (mLRho - mLZ) - mLY;
                    //                mLZdot = mLX * mLY - mLBeta * mLZ;
                    //
                    //                mLX += mLXdot * mTimeStep * 1005;
                    //                mLY += mLYdot * mTimeStep * 990;
                    //                mLZ += mLZdot * mTimeStep * 970;
                    //
                    //                noise = ((mLX * mLY) * 0.001 / (mTime * mAttack) - 0.05);
                    //                // used to be this: noise = ((mLX * mLY + mLY * mLZ + mLZ * mLX) * 0.01 * (mVelocity / 127.0) / (mTime * mAttack) - 0.05)) * 0.2;
                    
                    // Tinkerbell map:
                    
                    for (int l = 0; l < 2; l++) {
                        mTXnew = mTX * mTX - mTY * mTY + mTa * mTX + mTb * mTY;
                        mTYnew = 2 * mTY * mTX + mTc * mTX + mTd * mTY;
                        
                        mTX = mTXnew;
                        mTY = mTYnew;
                    }
                    
                    noise = (mTX) * 0.2 / (mTime * mAttack);
                    
                    
                    /// randomly pan the noisy transient
                    float randPan = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    //std::cout << "\nRandPan = " << randPan;
                    
                    samples[0] += noise * (1.0-randPan);
                    samples[1] += noise * randPan;
                    
                    //                samples[0] += noise;
                    //                samples[1] += noise;
                }
            }
            
            

            

            

            break;
            
            
        //////////////////////////---------/////////////////////////////
 
            
            
            
            
            
            
        case OSCILLATOR_MODE_SAW:

            samples[0] = samples[1] = 0;
            
            // original saw wave
            //value = 1.0 - (2.0 * mPhase / twoPI);
            break;
        case OSCILLATOR_MODE_SQUARE:
//            if (mPhase <= mPI) {
//                samples[0] = 1.0;
//            } else {
//                samples[0] = -1.0;
//            }
            samples[0] = samples[1] = 0;
            break;
        case OSCILLATOR_MODE_TRIANGLE:
//            samples[0] = -1.0 + (2.0 * mPhase / twoPI);
//            samples[0] = 2.0 * (fabs(samples[0]) - 0.5);
            samples[0] = samples[1] = 0;
            break;
        default:
            break;
    }
    //mPhase += mPhaseIncrement;
    // update time counter (seconds since last note trigger)
    mTime += mTimeStep;
    // update sample index (running index of which sample we're on since last note trigger)
    //mSampleIndex++;
//    while (mPhase >= twoPI) {
//        mPhase -= twoPI;
//    }
    //std::cout << "\nOscillator -> samples[0] = " << samples[0];

    return samples;
}