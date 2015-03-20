//
//  OpenCL.h
//  Synthesis
//
//  Created by Devin Mooers on 1/22/14.
//
//

#ifndef __Synthesis__OpenCL__
#define __Synthesis__OpenCL__

#define __NO_STD_VECTOR // Use cl::vector instead of STL vector
#define __NO_STD_STRING // Use cl::string instead of STL string
#define __CL_ENABLE_EXCEPTIONS

#include <math.h>
#include <boost/array.hpp>
#include <queue>
#include <utility>
#include <iostream>
#include <fstream>
#include <string>
//#include <boost/circular_buffer.hpp>
#include <OpenCL/cl.hpp>
using namespace cl;

#define BLOCK_SIZE 256
#define NUM_CHANNELS 2
#define MAX_VOICES 16
#define NUM_VOICE_PARAMS 5 // num params per voice - mTime, mFrequency, mVelocity, randStringMult, randomSeed
//#define numAuxiliaryParams 4
#define NUM_INSTRUMENT_PARAMS 7 // linear term, squared term, cubic term, brightness A, brightness B, pitch bend (coarse), pitch bend (fine)


class OpenCL {
public:
    friend class VoiceManager;
    OpenCL() :
    mTime(0.0f),
    sampleRate(44100.0f),
    NUM_PARTIALS(140),
    mStringDetuneRange(0.001f),
    mDamping(2.5f),
    mTimeStep(1.0f/44100)
//    instrumentData[0.3, 1.0f, 0.3f]
    {
        
        /// init variables
        
        //MIDIParams[0] = 0.0f;
    
        
        
//        try {
//            Platform::get(&platforms);
//            
//            // Select the default platform and create a context using this platform and the GPU
//            cl_context_properties cps[3] = {
//                CL_CONTEXT_PLATFORM,
//                (cl_context_properties)(platforms[0])(),
//                0
//            };
//            
//            context = cl::Context( CL_DEVICE_TYPE_GPU, cps);
//            
//            devices = context.getInfo<CL_CONTEXT_DEVICES>();
//            
//            //std::cout << "DEVICES = " << devices;
//            
//            // Print out details of the first device
//            std::cout << "\nSMOKE Device: " << devices[0].getInfo<CL_DEVICE_NAME>().c_str();
//            std::cout << "\nVersion: " << devices[0].getInfo<CL_DEVICE_VERSION>().c_str();
//            std::cout << "\nMax Compute Units: " << devices[0].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
//            std::cout << "\nGlobal Memory Size: " << devices[0].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()/1000000.0 << " MB";
//            std::cout << "\nLocal Memory Size: " << devices[0].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << " bytes";
//            std::cout << "\nMax Memory Object Size: " << devices[0].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << " bytes";
//            std::cout << "\nMax Work Group Size: " << devices[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
//            std::cout << "\nMax Work Item Dimensions: " << devices[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
//            //std::cout << "\nMax Work Item Sizes: " << devices[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
//            std::cout << "\nPreferred Vector Width (Float): " << devices[0].getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>();
//            std::cout << "\nPreferred Vector Width (Double): " << devices[0].getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>();
//            //std::cout << "\nExtensions: " << devices[0].getInfo<CL_DEVICE_EXTENSIONS>().c_str();
//            
//            
//            // Create a command queue and use the first device
//            queue = CommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE);
//            
//            // Read source file
//            std::ifstream sourceFile("opencl_kernels.cl");
//            std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
//            Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
//            
//            // Make program of the source code in the context
//            program = Program(context, source);
//            
//            // Build program for these specific devices
//            program.build(devices, "-cl-finite-math-only -cl-no-signed-zeros");
//            
//            string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
//            
//            std::cout << "\n\n\n" << log.c_str();
//            
//            // Make kernel
//            oscillatorKernel = Kernel(program, "oscillator");
//            addVoicesKernel = Kernel(program, "add_voices");
//
//        } catch(cl::Error error) {
//            std::cout << error.what() << "(" << error.err() << ")" << std::endl;
//            cl::STRING_CLASS buildlog;
//            buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
//            std::cout << "\n\n\n" << buildlog.c_str() << "\n\n\n";
//        }
    
        
    };
    void initOpenCL();
    inline boost::array<double, BLOCK_SIZE*NUM_CHANNELS> getBlockOfSamples() {
        calculateSamples();
        mTime += mTimeStep * BLOCK_SIZE;
        return blockOfSamples;
    }
    float voicesEnergy[MAX_VOICES*BLOCK_SIZE];

private:
    //void runOpenCL();
    //float *samples[128];
    boost::array<double, BLOCK_SIZE*NUM_CHANNELS> blockOfSamples;
    void calculateSamples();
    
    vector<Platform> platforms;
    Context context;
    vector<Device> devices;
    CommandQueue queue;
    Program program;
    Kernel oscillatorKernel;
    Kernel addVoicesKernel;
    
    short NUM_PARTIALS; // max number of partials to calculate for each note
    short NUM_ACTIVE_VOICES;
    
    //short NUM_CHANNELS;
    
    int GLOBAL_SIZE;
    int MAX_WORK_GROUP_SIZE;
    int WORK_GROUP_SIZE;
    
    float mTime, mTimeStep;
    float sampleRate;
    float voicesData[MAX_VOICES*NUM_VOICE_PARAMS];
    //float voicesDamping[MAX_VOICES*BLOCK_SIZE];
    float MIDIParams[3]; // sustain, expression, mod
    float mModPrevious, mModCurrent, mModSmoothed;
    std::queue<float> modBuffer;
//    boost::circular_buffer<float> modBuffer();
    
    float mB; // inharmonicity coefficient
    float mStringDetuneRange;
    float mPartialDetuneRange;
    float mDamping;
    
    float instrumentData[NUM_INSTRUMENT_PARAMS]; // linear term, squared term, cubic term
    
    Buffer voicesDataBuffer, voicesEnergyBuffer, instrumentDataBuffer, voicesSampleBuffer, outputSampleBuffer, ADSRBuffer;
    NDRange globalSize, localSize, globalSizeAdder, localSizeAdder;
};

#endif /* defined(__Synthesis__OpenCL__) */
