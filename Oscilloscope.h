//
//  Oscilloscope.h
//  Synthesis
//
//  Created by Devin Mooers on 1/16/14.
//
//

#ifndef __Synthesis__Oscilloscope__
#define __Synthesis__Oscilloscope__

//#ifdef OS_OSX
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#include <iostream>
#include <stdlib.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-tokens"
#include "IPlug_include_in_plug_hdr.h"
#pragma clang diagnostic pop
#include "IControl.h"
#include <boost/circular_buffer.hpp>



class Oscilloscope : public IControl {
    
public:
    Oscilloscope(IPlugBase* pPlug, IRECT pR) :
    IControl(pPlug, pR, -1),
    bufferSize(1764), // works pretty well with 98 and % 18, or 196/9
    mLastSample(0.0),
    xPos(1),
    xOffset(0),
    yOffset(0),
    yOffsetNext(0),
    yOffsetEnd(0),
    //cb(cbSize), // as many elements in ring buffer as there are pixels in width of oscilloscope
    rgbR(55),
    rgbG(255),
    rgbB(55),
    iterator(0),
    bufferFull(false)
    //width(300),
    //height(300)
    //xMultiplier(OSCILLOSCOPE_WIDTH / cbSize)
    {
        initBuffer();
        createContext();
        //mData.Resize(mRECT.W() * mRECT.H() * 4);
        updatePixelColors();
    }
    
    ~Oscilloscope() {
        destroyContext();
    }
    
    void updateLastSample(double sampleL, double sampleR);
    long setContext();
    void restoreContext();
    long createContext();
    void destroyContext();
    bool Draw(IGraphics* pGraphics);
    bool IsDirty();
    double mLastSample;
    void updatePixelColors();
    
private:
    CGLContextObj mGLContext;
    WDL_TypedBuf<unsigned char> mData;
    float mRotateTri, mRotateQuad;
    int xPos;
    int halfHeight;
    int xOffset;
    int yOffset;
    int yOffsetNext;
    int yOffsetEnd;
    int rgbR;
    int rgbG;
    int rgbB;
    int x;
    const int bufferSize;
    //int width;
    //int height;
    //double xMultiplier;
    //boost::circular_buffer<double> cb;
    double buffer [2][1764];
    bool bufferFull;
    int iterator;
    void initBuffer();
    LICE_pixel pixBlack;
    LICE_pixel pixGreen;
    LICE_pixel pixLightGreen;
    LICE_pixel pixWhite;
};
//#endif


#endif /* defined(__Synthesis__Oscilloscope__) */
