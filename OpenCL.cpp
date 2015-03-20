//
//  OpenCL.cpp
//  Synthesis
//
//  Created by Devin Mooers on 1/22/14.
//
//



#define __NO_STD_VECTOR // Use cl::vector instead of STL vector
#define __NO_STD_STRING // Use cl::string instead of STL string
#define __CL_ENABLE_EXCEPTIONS

#include "OpenCL.h"

void CL_CALLBACK ping(cl_event event, cl_int status, void* data) {
    std::cout << "\nCallback triggered - PING!";
}


void OpenCL::initOpenCL() {
    mTime = 0.0f;
    sampleRate = 44100.0f;
    NUM_PARTIALS = 64;
    mTimeStep = 1.0f/sampleRate;
    MIDIParams[0] = 0.0f;

    
    try {
        Platform::get(&platforms);
        
        // Select the default platform and create a context using this platform and the GPU
        cl_context_properties cps[3] = {
            CL_CONTEXT_PLATFORM,
            (cl_context_properties)(platforms[0])(),
            0
        };
        
        context = Context( CL_DEVICE_TYPE_GPU, cps);
        
        devices = context.getInfo<CL_CONTEXT_DEVICES>();
        
//        std::cout << "DEVICES = " << devices[0].getInfo<CL_DEVICE_NAME>().c_str() << ", " << devices[1].getInfo<CL_DEVICE_NAME>().c_str() << "\n\n";
        
        bool verbose = false;
        
        if (verbose) {
            // Print out details of the first device
            std::cout << "\nDevice: " << devices[0].getInfo<CL_DEVICE_NAME>().c_str();
            std::cout << "\nVersion: " << devices[0].getInfo<CL_DEVICE_VERSION>().c_str();
            std::cout << "\nMax Compute Units: " << devices[0].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
            std::cout << "\nGlobal Memory Size: " << devices[0].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()/1000000.0 << " MB";
            std::cout << "\nLocal Memory Size: " << devices[0].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << " bytes";
            std::cout << "\nMax Memory Object Size: " << devices[0].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << " bytes";
            std::cout << "\nMax Work Group Size: " << devices[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
            std::cout << "\nMax Work Item Dimensions: " << devices[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
            std::cout << "\nMax Work Item Sizes: " << devices[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
            std::cout << "\nPreferred Vector Width (Float): " << devices[0].getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>();
            std::cout << "\nPreferred Vector Width (Double): " << devices[0].getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>();
            std::cout << "\nExtensions: " << devices[0].getInfo<CL_DEVICE_EXTENSIONS>().c_str();
        }
        
        
        
        
        // Create a command queue and use the first device
        // queue = CommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE);
        // Let's try using the second GPU on the Mac Pro!
        queue = CommandQueue(context, devices[1], CL_QUEUE_PROFILING_ENABLE);
        
        // Read source file
        std::ifstream sourceFile("opencl_kernels.cl");
        std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
        Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
        
        // Make program of the source code in the context
        program = Program(context, source);
                
        // Build program for these specific devices
        program.build(devices, "-cl-finite-math-only -cl-no-signed-zeros");
        
        string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
        
        std::cout << "\n\n\n" << log.c_str();
        
        // Make kernel
        oscillatorKernel = Kernel(program, "oscillator");
        addVoicesKernel = Kernel(program, "add_voices");
        
        printf("Kernels compiled successfully!");
        
    } catch(Error error) {
        std::cout << "\nline 92 .cpp\n";
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
        cl::STRING_CLASS buildlog;
        buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
        std::cout << "\n\n\n" << buildlog.c_str() << "\n\n\n";
    }
}

void OpenCL::calculateSamples() {
    
    try {
        
//        // copy voice data into array for loading to buffer
//        for(int i = 0; i < NUM_ACTIVE_VOICES*NUM_VOICE_PARAMS; i++) {
//            A[i] = voicesData[i];
//            //std::cout << "\nvoicesData " << i << " = " << voicesData[i];
//        }
                
        voicesDataBuffer = Buffer(context, CL_MEM_READ_ONLY, NUM_ACTIVE_VOICES * NUM_VOICE_PARAMS * sizeof(float));

        Event profileEventWriteBuffer;
        //cl_ulong start, end;

        // Copy voiceData to memory buffer for writing to device
        queue.enqueueWriteBuffer(voicesDataBuffer, CL_FALSE, 0, NUM_ACTIVE_VOICES * NUM_VOICE_PARAMS * sizeof(float), &voicesData, NULL, &profileEventWriteBuffer);
        
        
        // create energy buffer and enqueue it for writing to GPU memory
        voicesEnergyBuffer = Buffer(context, CL_MEM_READ_WRITE, BLOCK_SIZE * NUM_ACTIVE_VOICES * sizeof(float));
        queue.enqueueWriteBuffer(voicesEnergyBuffer, CL_FALSE, 0, NUM_ACTIVE_VOICES * BLOCK_SIZE * sizeof(float), &voicesEnergy, NULL, NULL);
        
//        start = profileEventWriteBuffer.getProfilingInfo<CL_PROFILING_COMMAND_START>();
//        end = profileEventWriteBuffer.getProfilingInfo<CL_PROFILING_COMMAND_END>();
//        std::cout << "\nElapsed time (write buffer): " << (end-start)/1000.0 << " μs";
        
        voicesSampleBuffer = Buffer(context, CL_MEM_READ_WRITE, BLOCK_SIZE * NUM_CHANNELS * sizeof(float) * NUM_ACTIVE_VOICES); // intermediate output buffer storing one block of samples per voice (e.g. 64 blocks of 512 samples) which will get added by adder kernel later
        
        
        // extra instrument data (could merge mB, mStringDetuneRange, etc. into this array! WAY cleaner!)
        instrumentDataBuffer = Buffer(context, CL_MEM_READ_ONLY, NUM_INSTRUMENT_PARAMS * sizeof(float));
        queue.enqueueWriteBuffer(instrumentDataBuffer, CL_FALSE, 0, NUM_INSTRUMENT_PARAMS * sizeof(float), &instrumentData, NULL, NULL);
        
        
//        Buffer MIDIParamsBuffer = Buffer(context, CL_MEM_READ_WRITE, 1 * sizeof(float));
//        queue.enqueueWriteBuffer(MIDIParamsBuffer, CL_FALSE, 0, 1 * sizeof(float), &MIDIParams);
        
        /// testing MIDI
        
        //printf("\nsustain = %f", MIDIParams[0]);
//        printf("%f\n", MIDIParams[2]);
//        printf("%f\n", mModCurrent);
        
        
        
        // Calculate smoothed mod wheel values
        // add new value to modBuffer and calculate SMA
        modBuffer.push(mModCurrent);
        // add the newest value and subtract out the oldest value from SMA (if we have more than X values in there already)
        mModSmoothed += (mModCurrent - modBuffer.back()) / modBuffer.size();
        if (modBuffer.size() > 4) {
            modBuffer.pop();
        }
//        printf("modBuffer length: %d\n", (int)modBuffer.size());
//        printf("%f\n", mModSmoothed);
        
        
//        float mModDelta = mModCurrent - mModPrevious;
//        printf("mModDelta = %f", mModDelta);
        
//        printf("voicesData:\n");
//        for (int i = 0; i < 5; i++) {
//            std::cout << voicesData[i] << std::endl;
//        }
//        printf("------\n");

        
        // Set arguments to kernel
        oscillatorKernel.setArg(0, voicesDataBuffer);
        oscillatorKernel.setArg(1, voicesEnergyBuffer);
        oscillatorKernel.setArg(2, instrumentDataBuffer);
        oscillatorKernel.setArg(3, mModPrevious); // mod wheel
        oscillatorKernel.setArg(4, mModCurrent);
        oscillatorKernel.setArg(5, mB);
        oscillatorKernel.setArg(6, mPartialDetuneRange);
        oscillatorKernel.setArg(7, mTimeStep);
        oscillatorKernel.setArg(8, NUM_PARTIALS);
        oscillatorKernel.setArg(9, BLOCK_SIZE);
        oscillatorKernel.setArg(10, NUM_CHANNELS);
        oscillatorKernel.setArg(11, voicesSampleBuffer);
        
        mModPrevious = mModCurrent;
        
        /// test print out mEnergy array
//        for (int i = 0; i < NUM_ACTIVE_VOICES * BLOCK_SIZE; i++) {
//            printf("%f\n", voicesEnergy[i]);
//        }
        
        
        
//        std::cout << "\nMax work-items per work-group for this kernel: " << oscillatorKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(devices[0]); /// warning: I think this is OpenCL 1.1 only.
//        std::cout << "\nPreferred work-group size multiple for this kernel: " << oscillatorKernel.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(devices[0]); /// warning: I think this is OpenCL 1.1 only.
//        std::cout << "\nLocal memory size used by this kernel: " << oscillatorKernel.getWorkGroupInfo<CL_KERNEL_LOCAL_MEM_SIZE>(devices[0]);
//        std::cout << "\nPrivate memory size used by this kernel: " << oscillatorKernel.getWorkGroupInfo<CL_KERNEL_PRIVATE_MEM_SIZE>(devices[0]);
        
        GLOBAL_SIZE = BLOCK_SIZE * NUM_ACTIVE_VOICES; // each work-item calculates one stereo (or mono) sample for one voice
        globalSize = GLOBAL_SIZE; // NDRange var
        
        // find max work group size supported on the device for this kernel.
        /// NOTE: OpenCL 1.1 only, I think. Later on, default this to 256, and then say, hey, if you have OpenCL 1.1 or above, then check what the max work group size is for this kernel, then use THAT number. As a fallback, use 256.
        MAX_WORK_GROUP_SIZE = static_cast<int>(oscillatorKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(devices[0]));
        WORK_GROUP_SIZE = MAX_WORK_GROUP_SIZE;
        
        
        // make sure local size isn't bigger than global size
        // e.g. if only doing 1 synth voice with 128 samples, global size would be 128 but max work group size could be 256, causing -54 error (can't have local size bigger than global size)
        if (WORK_GROUP_SIZE > GLOBAL_SIZE) {
            WORK_GROUP_SIZE = GLOBAL_SIZE;
        }
        // this is to make sure that global size is always divisible by local size (to avoid -54 error, e.g. globalsize 384 and local size 256)
        if (GLOBAL_SIZE%WORK_GROUP_SIZE > 0) {
            WORK_GROUP_SIZE = BLOCK_SIZE;
        }
        localSize = WORK_GROUP_SIZE; // NDRange var
        
        //printf("global size = %d, local size = %d\n", GLOBAL_SIZE, WORK_GROUP_SIZE);
        
        // Create profiling event
        Event profileEvent;
        Event profileEventAdder;
        Event profileEventRead;
        // Configure callback event
        Event callbackEvent;
        
        
        /// launch oscillator kernel
        
        /// architecture: each work-item computes one sample for one voice. they do not talk to each other, so feedback is not possible.
        
        // Compute!
        //printf("just before oscillatorkernel");
        queue.enqueueNDRangeKernel(oscillatorKernel, NullRange, globalSize, localSize, NULL, &profileEvent); // second arg is offset
//
        
//        std::cout << "\nNumber of voices: " << NUM_ACTIVE_VOICES;
//        std::cout << "\nNumber of partials per voice: " << NUM_PARTIALS;
//        std::cout << "\nTotal number of partials: " << NUM_PARTIALS * NUM_ACTIVE_VOICES;

        /// elapsed oscillator kernel time
        
//        queue.finish();
//        cl_ulong start, end;
//        start = profileEvent.getProfilingInfo<CL_PROFILING_COMMAND_START>();
//        end = profileEvent.getProfilingInfo<CL_PROFILING_COMMAND_END>();
//        std::cout << "\nElapsed time (oscillator): " << (int)((end-start)/1000.0) << " μs";
//        // only print the bad ones
//        //        if ((end-start)/1000.0 > 500) {
//        //            std::cout << "\n*****Elapsed time (oscillator): " << (end-start)/1000.0 << " μs";
//        //        }
//        std::cout << "\nOne block of samples time length: " << mTimeStep*BLOCK_SIZE*1000000 << " μs";
        
        
        
        /// launch a final adder kernel
        
        /// this kernel adds up the array of samples for each voice - e.g. we have 16 blocks of 512 samples (256 * 2 channels), and each work-item is going to add up one sample index, e.g. 0 + 512 + 1024 + 1536...

        float *samples = new float[BLOCK_SIZE * NUM_CHANNELS];
        
        outputSampleBuffer = Buffer(context, CL_MEM_READ_WRITE, BLOCK_SIZE * NUM_CHANNELS * sizeof(float));
        
        addVoicesKernel.setArg(0, voicesSampleBuffer);
        addVoicesKernel.setArg(1, NUM_ACTIVE_VOICES);
        addVoicesKernel.setArg(2, BLOCK_SIZE);
        addVoicesKernel.setArg(3, NUM_CHANNELS);
        addVoicesKernel.setArg(4, outputSampleBuffer);
        
        MAX_WORK_GROUP_SIZE = static_cast<int>(addVoicesKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(devices[0]));

        int global_size_adder = BLOCK_SIZE * NUM_CHANNELS;
        globalSizeAdder = global_size_adder;
        int local_size_adder = MAX_WORK_GROUP_SIZE;
        // make sure that local size isn't bigger than global size
        if (local_size_adder > global_size_adder) {
            local_size_adder = global_size_adder;
        }
        localSizeAdder = local_size_adder;
        
        queue.enqueueNDRangeKernel(addVoicesKernel, NullRange, globalSizeAdder, localSizeAdder, NULL, &profileEventAdder); // second arg is offset
        
        
//        start = profileEventAdder.getProfilingInfo<CL_PROFILING_COMMAND_START>();
//        end = profileEventAdder.getProfilingInfo<CL_PROFILING_COMMAND_END>();
//        std::cout << "\nElapsed time (adder kernel): " << (end-start)/1000.0 << " μs";
        
        // read back final summed samples
        queue.enqueueReadBuffer(outputSampleBuffer, CL_TRUE, 0, BLOCK_SIZE * NUM_CHANNELS * sizeof(float), samples, NULL, &profileEventRead);
        
        //queue.finish();
        
//        start = profileEventRead.getProfilingInfo<CL_PROFILING_COMMAND_START>();
//        end = profileEventRead.getProfilingInfo<CL_PROFILING_COMMAND_END>();
//        std::cout << "\nElapsed time (read buffer): " << (end-start)/1000.0 << " μs";
        
        
        // Set callback function
        //callbackEvent.setCallback(CL_COMPLETE, &ping, (void*)samples);
        
        // Copy output buffer to blockOfSamples for returning
        for (int i = 0; i < BLOCK_SIZE * NUM_CHANNELS; i++) {
            blockOfSamples[i] = (double)samples[i];
            
//            // add in bandlimited noise
//            blockOfSamples[i/2] += mFilterL.process(0.01f * (static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/2.0f)) - 1.0f));
//            blockOfSamples[i/2+1] += mFilterR.process(0.01f * (static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/2.0f)) - 1.0f));
//            if (isnan(blockOfSamples[i])) {
//                blockOfSamples[i] = 0.0;
//            }
            //std::cout << "\nSample " << i/2 << " = " << blockOfSamples[i];
        }
        
        
    } catch(Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
//        cl::STRING_CLASS buildlog;
//        buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
//        std::cout << "\n\n\n" << buildlog.c_str() << "\n\n\n";
    }
}