//
//  Oscilloscope.cpp
//  Synthesis
//
//  Created by Devin Mooers on 1/16/14.
//
//

#include "Oscilloscope.h"
#include "IControl.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#include <math.h>
#include <boost/circular_buffer.hpp>



void Oscilloscope::restoreContext() {
    CGLSetCurrentContext(NULL);
}

long Oscilloscope::createContext() {
    const GLubyte *glstring;
    
    GLint npix;
    CGLPixelFormatObj PixelFormat;
    
    const CGLPixelFormatAttribute attributes[] = {
        //kCGLPFAOffScreen,
        //      kCGLPFAColorSize, (CGLPixelFormatAttribute)8,
        //      kCGLPFADepthSize, (CGLPixelFormatAttribute)16,
        kCGLPFAAccelerated, (CGLPixelFormatAttribute)0
    };
    
    // Create context if none exists
    CGLChoosePixelFormat(attributes, &PixelFormat, &npix);
    
    if (PixelFormat == NULL) {
        DBGMSG("Could not get pixel format.");
        return 1;
    }
    
    CGLCreateContext(PixelFormat, NULL, &mGLContext);
    
    if (mGLContext == NULL) {
        DBGMSG("Could not create rendering context.");
        return 1;
    }
    
    // Set the current context
    if(setContext())
        return 1;
    
    // Check OpenGL functionality:
    glstring = glGetString(GL_EXTENSIONS);
    
    if(!gluCheckExtension((const GLubyte *)"GL_EXT_framebuffer_object", glstring)) {
        DBGMSG("The GL_EXT_framebuffer_object extension is not supported on this system.");
        return 1;
    }
    restoreContext();
    return 0;
}

void Oscilloscope::destroyContext() {
    if (mGLContext != NULL)
        CGLDestroyContext(mGLContext);
}

long Oscilloscope::setContext() {
    // Set the current context
    if(CGLSetCurrentContext(mGLContext)) {
        DBGMSG("Could not make context current.");
        return 1;
    }
    return 0;
}

void Oscilloscope::initBuffer() {
    //boost::circular_buffer<double> cb(100);
    for (int i = 0; i < bufferSize; i++) {
        //cb.push_back(0.0);
        buffer[0][i] = buffer[1][i] = 0;
    }
}

void Oscilloscope::updatePixelColors() {
    //pixBlack = LICE_RGBA(0, 0, 0, 255);
    //pixGreen = LICE_RGBA(rgbR, rgbG, rgbB, 255);
    //pixLightGreen = LICE_RGBA(200,255,200,255);
    //pixWhite = LICE_RGBA(255,255,255,255);
//    rgbR = rand() % 128 + 128;
//    rgbG = rand() % 128 + 128;
//    rgbB = rand() % 128 + 128;
//    pixGreen = LICE_RGBA(255, 60, 10, 255);
    pixGreen = LICE_RGBA(55, 255, 55, 255);
}

void Oscilloscope::updateLastSample(double sampleL, double sampleR) {
    //cb.push_back(sample);
    if (iterator > (bufferSize - 1)) {
        iterator = 0;
    }
    buffer[0][iterator] = sampleL;
    buffer[1][iterator] = sampleR;
    iterator++;
}

bool Oscilloscope::Draw(IGraphics* pGraphics) {

    // render buffer example: https://github.com/olilarkin/wdl-ol/blob/master/IPlugExamples/IPlugOpenGL/IPlugOpenGL.h
    
    //const int width = mRECT.W();
    //const int width = mRECT.W();//600;//bufferSize * 3;
    //const int height = mRECT.H();
    
    // Set context
    if (setContext())
        return false;
    
    // Reset The Current Viewport
    restoreContext();
    
    /// display pixels on screen
    
    //std::cout << pGraphics->FPS() << " fps\n";
    
    // draw black pixels over the whole screen (or would just drawing a black rectangle be faster? probably... or just a "clear screen" option?
//    for (int y = 0; y < height; y++) {
//        for (int x = 0; x < width; x++) {
//            LICE_PutPixel(pGraphics->GetDrawBitmap(), mRECT.L + x, mRECT.B - y, pixBlack, 0.1, LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA);
//        }
//    }
    
    int xPos = 500;
    int yPos = 300;
    int xMult = 4000; // 16000 works well, doesn't overflow 1000x1000; 20000 is bigger, but clips a little
    int yMult = 1000; // 4000 works well, doesn't overflow 1000x1000; 6000 is bigger, but clips a little
    
    
    /// phase scope / Lissajous
    
    for (int a = 0; a < bufferSize-1; a++) {
        int xPhaseOffset = mRECT.L + xPos + xMult*log((buffer[0][a]-buffer[1][a])+1); // making Lissajous plot vertical instead of angled
        int yPhaseOffset = mRECT.B - yPos - yMult*log(buffer[1][a]+1);
        int xPhaseOffset2 = mRECT.L + xPos + xMult*log((buffer[0][a+1]-buffer[1][a+1])+1); // making Lissajous plot vertical instead of angled
        int yPhaseOffset2 = mRECT.B - yPos - yMult*log(buffer[1][a+1]+1);
    
        // points
        LICE_PutPixel(pGraphics->GetDrawBitmap(), xPhaseOffset, yPhaseOffset, pixGreen, 0.4, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
        LICE_PutPixel(pGraphics->GetDrawBitmap(), xPhaseOffset-1, yPhaseOffset, pixGreen, 0.2, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
        LICE_PutPixel(pGraphics->GetDrawBitmap(), xPhaseOffset+1, yPhaseOffset, pixGreen, 0.2, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
        LICE_PutPixel(pGraphics->GetDrawBitmap(), xPhaseOffset, yPhaseOffset-1, pixGreen, 0.2, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
        LICE_PutPixel(pGraphics->GetDrawBitmap(), xPhaseOffset, yPhaseOffset+1, pixGreen, 0.2, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
        
        // lines
        LICE_Line(pGraphics->GetDrawBitmap(), xPhaseOffset, yPhaseOffset, xPhaseOffset2, yPhaseOffset2, pixGreen, 0.1, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
        
    }
    
    
    
    
    /// bezier
    
//    for (int a = 0; a < bufferSize - 2; a+=2) {
//        
//        x = a;
//        xOffset = mRECT.L + x + 50;
//        yOffset = mRECT.B - 150 - 1000*buffer[0][a];
//        yOffsetNext = mRECT.B - 150 - 1000*buffer[0][a+1];
//        yOffsetEnd = mRECT.B - 150 - 1000*buffer[0][a+2];
//        
//        LICE_DrawQBezier(pGraphics->GetDrawBitmap(), xOffset, yOffset, xOffset+1, yOffsetNext, xOffset+2, yOffsetEnd, pixLightGreen, 0.4, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
//        LICE_DrawQBezier(pGraphics->GetDrawBitmap(), xOffset, yOffset+1, xOffset+1, yOffsetNext+1, xOffset+2, yOffsetEnd+1, pixGreen, 0.4, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
//        LICE_DrawQBezier(pGraphics->GetDrawBitmap(), xOffset, yOffset-1, xOffset+1, yOffsetNext-1, xOffset+2, yOffsetEnd-1, pixGreen, 0.4, LICE_BLIT_USE_ALPHA | LICE_BLIT_MODE_ADD);
//        
//    }
    
    
    
    

    return true;
}

bool Oscilloscope::IsDirty() {
    return true;
}
