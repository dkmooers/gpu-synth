__kernel void oscillator(__global const float *voicesDataBuffer,
                         __global const float *voicesEnergyBuffer,
                         __global const float *instrumentDataBuffer,
                         float mModPrevious,
                         float mModCurrent,
                         float mB,
                         float partialDetuneRange,
                         float mTimeStep,
                         short NUM_PARTIALS,
                         short BLOCK_SIZE,
                         short NUM_CHANNELS,
                         __global float *voicesSampleBuffer
                         ) {
    
    int globalID = get_global_id(0);
    short voiceID = globalID / BLOCK_SIZE; // find which voice # this work-item is calculating a sample for
    float mTime = voicesDataBuffer[voiceID*5];
    int sampleIndex = globalID - (BLOCK_SIZE*voiceID);// sample index/offset within this voice (never higher than BLOCK_SIZE-1)
    mTime += mTimeStep * (float)sampleIndex; // find actual time value for this sample
    float mFrequency = voicesDataBuffer[voiceID*5+1];
    float mVelocity = (float)voicesDataBuffer[voiceID*5+2];
    float randStringMult = voicesDataBuffer[voiceID*5+3];
    short x = (short)voicesDataBuffer[voiceID*5+4]; // random seed
    float mEnergy = voicesEnergyBuffer[voiceID*BLOCK_SIZE + sampleIndex];
    
    // re-center mod wheel values around 0
//    mModPrevious = mModPrevious - 0.5f;
//    mModCurrent = mModCurrent - 0.5f;
        
    // linear
//    float mMod = mModPrevious + sampleIndex * (mModCurrent - mModPrevious) / (BLOCK_SIZE - 1);
    
    float xVal = convert_float(sampleIndex)/convert_float(BLOCK_SIZE); // scale sampleIndex/BLOCK_SIZE to (0,1) value
    
    // smoothstep
    xVal = xVal * xVal * (3.0f - 2.0f * xVal);
    float mMod = mModPrevious + (mModCurrent - mModPrevious) * xVal; // the first bit re-scales the output, and the last bit is the cubic interp function
    
    // smootherstep
//    xVal = xVal*xVal*xVal*(xVal*(xVal*6.0f - 15.0f) + 10.0f);
//    float mMod = mModPrevious + (mModCurrent - mModPrevious) * xVal;
    
    
    // re-center mod around 0
    mMod = mMod - 0.5f;
    
    // no interpolation
//    float mMod = mModCurrent - 0.5f;
    
    
    float mLinearTerm = instrumentDataBuffer[0];
    float mSquaredTerm = instrumentDataBuffer[1];
    float mCubicTerm = instrumentDataBuffer[2];
    float mBrightnessA = 1.0f - instrumentDataBuffer[3];
    float mBrightnessB = 10000.0f * instrumentDataBuffer[4];
    
    float mPitchBendCoarse = 2.0f * instrumentDataBuffer[5]; // (0, 2)
//    mPitchBendCoarse += 0.03f * mMod; // use mod wheel as pitch bend for smooth vibrato testing
    float mPitchBendFine = 0.02f * instrumentDataBuffer[6]; // (0, 0.02)
//    mPitchBendFine += (0.02f * (mMod)); // use mod wheel as pitch bend for smooth vibrato testing
    
//    mPitchBendCoarse = 1.0f;
//    mPitchBendFine = 0.01f;
    
//    if (sampleIndex == 0) {
//        printf("\n----------\n");
//    }
//    printf("%f\n", mMod);
    
    if ((int)(22050.0f / mFrequency) < NUM_PARTIALS) { // was 22050
        NUM_PARTIALS = (int)(22050.0f / mFrequency); // was 22050
    }; // scale down num_partials to only the MAX number actually needed for this note, but don't go over the MAX partials (e.g. 128), since a 50Hz note, for example, would need 441 partials!
    mB *= 0.1f + mFrequency/10000.0f; // make the apparent effect of mB more linear across the octaves, so it's smaller for low notes and higher for high notes
    mB *= 0.1f + mFrequency*mFrequency/50000000.0f; // make the apparent effect of mB more linear across the octaves, so it's smaller for low notes and higher for high notes... in an exponential fashion, so gets much larger faster with higher frequencies
    mB *= 1.01f / (1.01f - (mVelocity/(1.0f+mTime*10.0f)) / 5.0f); // scales mB with velocity -- the harder you hit, the more non-linear the partials become! but this effect only lasts a brief moment (while the string is majorly deformed from the impact). // mTime's multiplicand governs how fast mB changes in response to velocity -- 1.0f is slow drop, 10.0f is much faster drop. // the final divisor governs how STRONGLY mB changes in response to velocity. 1.0 is fairly strong, while 2.0 is not nearly as strong, and 10.0 is hardly any change at all.
    
    // VECTOR VERSION (FLOAT4)
    
    float4 freqs;
    float4 valuesOne;
    float4 valuesTwo;
    float4 eyes;
    float4 amps;
    
    float sampleL = 0.0f;
    float sampleR = 0.0f;
    
    float4 rands;
    
    //float partialDetuneRange = 1.0f; // modulate the 1.0f to be the partialDetuneRange knob - 0.5f is very subtle, 2.0f is much bigger.
    
    partialDetuneRange /= 7000000.0f;
    
    for (int i = 0; i < NUM_PARTIALS; i+=4) {
        
        // xorshift deterministic RNG
        // generate 4 random numbers (to use as partial frequency multipliers and random pan values)
        
        x = x ^ (x << 21);
        x = x ^ (x >> 35);
        x = x ^ (x << 4);
        rands.s0 = (float)x * partialDetuneRange + 1.0f; // 10,000,000 is very subtle
        x = x ^ (x << 21);
        x = x ^ (x >> 35);
        x = x ^ (x << 4);
        rands.s1 = (float)x * partialDetuneRange + 1.0f;
        x = x ^ (x << 21);
        x = x ^ (x >> 35);
        x = x ^ (x << 4);
        rands.s2 = (float)x * partialDetuneRange + 1.0f;
        x = x ^ (x << 21);
        x = x ^ (x >> 35);
        x = x ^ (x << 4);
        rands.s3 = (float)x * partialDetuneRange + 1.0f;
    
        eyes.s0 = (float)i + 1.0f;
        eyes.s1 = (float)i + 2.0f;
        eyes.s2 = (float)i + 3.0f;
        eyes.s3 = (float)i + 4.0f;
        

        // testing mod here
//        mFrequency += mMod; // this makes the frequency change with the mod wheel ALSO (mod wheel already changes timbre/brightness a bit)
        
        // mod wheel changes brightness/timbre subtly here
        freqs = (mPitchBendCoarse + mPitchBendFine * pow(eyes, 0.3f)) * eyes * mFrequency * sqrt((1.0f + mB * eyes * eyes)); // includes inharmonicity coefficient
        
//        printf("%f, %f\n", mFrequency, freqs.s3);
        
        amps = pow((mEnergy*(mLinearTerm + mEnergy*(mSquaredTerm + mEnergy*(mCubicTerm)))), (pow(eyes, mBrightnessA)+freqs/mBrightnessB));
        
        // testing timbre envelope
//        amps += 0.2f * sin(freqs / 5000.0f + mMod) * mEnergy / eyes;
        
        
        // calculate string 1 and 2
        // 2pi used to be: 6.283185307179586f (but too many digits for float!)
        valuesOne = sin(6.2831853f * mTime * freqs * rands) * amps * ((1.0f-rands)*(75.0f/partialDetuneRange/7000000.0f)+0.7f); // the last multiplier, using rands, is for random partial amplitudes, using same rands as for partial frequency multipliers
        valuesTwo = sin(6.2831853f * mTime * freqs * rands * randStringMult) * amps * ((1.0f-rands)*(75.0f/partialDetuneRange/7000000.0f)+0.7f);
        
        // random white noise transient test
        
        //if (mTime < 0.100f) {
        valuesOne.s0 += fabs(rands.s0 - 1.0f) * pow(0.5f, mTime*50.0f) * 20.0f * mEnergy * mEnergy * (1.0f + mEnergy);
        //}

            //rands = fabs(rands-1.0f) * 214.0f; // subtract 1 to center around 0, take abs so it's positive, and make it bigger - want it be (0,1) -->
        rands = fabs(rands-1.0f) * 214.0f / partialDetuneRange / 7000000.0f; // subtract 1 to center around 0, take abs so it's positive, and make it bigger - want it be (0,1) --> this undoes the multiplication above at partialDetuneRange and scales the rands appropriately so they're (0,1) for L/R pan values


            
        /// reverb wash (sound starts in mono and washes out to the sides, to random pan positions, as if traveling along the soundboard)
        

        
        // pan speed multiplier
        eyes.s0 = 5.0f * mTime + 1.0f; // I'm just re-using a float variable here to save memory -- nothing relevant about eyes in these following calculations // Also, 5.0f is the pan speed multiplier -- the higher that number, the faster the pans will wash out to the sides. 5.0f is a good medium value, not too fast, not too slow. Subtle, complex, realistic.

        // add string 1 to left channel
        sampleL += valuesOne.s0 * ((1.0f-rands.s0) - (0.5f - rands.s0)/eyes.s0);
        sampleL += valuesOne.s1 * ((1.0f-rands.s1) - (0.5f - rands.s1)/eyes.s0);
        sampleL += valuesOne.s2 * ((1.0f-rands.s2) - (0.5f - rands.s2)/eyes.s0);
        sampleL += valuesOne.s3 * ((1.0f-rands.s3) - (0.5f - rands.s3)/eyes.s0);
        // add string 2 to left channel
        sampleL += valuesTwo.s0 * ((1.0f-rands.s2) - (0.5f - rands.s2)/eyes.s0);
        sampleL += valuesTwo.s1 * ((1.0f-rands.s0) - (0.5f - rands.s0)/eyes.s0);
        sampleL += valuesTwo.s2 * ((1.0f-rands.s3) - (0.5f - rands.s3)/eyes.s0);
        sampleL += valuesTwo.s3 * ((1.0f-rands.s1) - (0.5f - rands.s1)/eyes.s0);
        
        // add string 1 to right channel
        sampleR += valuesOne.s0 * ((rands.s0) - (rands.s0 - 0.5f)/eyes.s0);
        sampleR += valuesOne.s1 * ((rands.s1) - (rands.s1 - 0.5f)/eyes.s0);
        sampleR += valuesOne.s2 * ((rands.s2) - (rands.s2 - 0.5f)/eyes.s0);
        sampleR += valuesOne.s3 * ((rands.s3) - (rands.s3 - 0.5f)/eyes.s0);
        // add string 2 to right channel
        sampleR += valuesTwo.s0 * ((rands.s2) - (rands.s2 - 0.5f)/eyes.s0);
        sampleR += valuesTwo.s1 * ((rands.s0) - (rands.s0 - 0.5f)/eyes.s0);
        sampleR += valuesTwo.s2 * ((rands.s3) - (rands.s3 - 0.5f)/eyes.s0);
        sampleR += valuesTwo.s3 * ((rands.s1) - (rands.s1 - 0.5f)/eyes.s0);
        


    }
    
    // write this work-item's sample to global memory
    // only works in stereo (include an if statement to switch between stereo and mono)
    voicesSampleBuffer[NUM_CHANNELS * (BLOCK_SIZE * voiceID + sampleIndex)] = sampleL * 0.15f; // was * 0.03f when using amps w/o mEnergy
    voicesSampleBuffer[NUM_CHANNELS * (BLOCK_SIZE * voiceID + sampleIndex) + 1] = sampleR * 0.15f; // was * 0.03f when using amps w/o mEnergy
}


__kernel void add_voices(__global float *voicesSampleBuffer, short NUM_ACTIVE_VOICES, short BLOCK_SIZE, short NUM_CHANNELS, __global float *outputSampleBuffer) {
    
    int globalID = get_global_id(0);
    float sample = 0.0f;
    
    // this adder kernel - first work-item adds up first sample of left channel of voice 1, voice 2, etc., second work-item adds up first sample of right channel of voice 1, voice 2, etc.,...
    
    for (short i = 0; i < NUM_ACTIVE_VOICES; i++) {
        sample += voicesSampleBuffer[i * BLOCK_SIZE * NUM_CHANNELS + globalID];
    }
    // write back to global memory
    outputSampleBuffer[globalID] = sample;
}