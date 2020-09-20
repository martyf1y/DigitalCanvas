/*
*  ProcessFilters.h
*
*
*  Created on 2/2/09.
*  Copyright 2009 NUI Group. All rights reserved.
*
*/

#ifndef PROCESS_FILTERS_H_
#define PROCESS_FILTERS_H_

#include "Filters.h"

extern bool bRevealSettings;

class ProcessFilters : public Filters {
public:

    void allocate( int w, int h ) {
        
        camWidth = w;
        camHeight = h;
        
		fLearnRate = 0.0f;
       
        
		exposureStartTime = ofGetElapsedTimeMillis();
        
       
        grayBg.allocate(camWidth, camHeight);		//  Background Image
      
        grayDiff.allocate(camWidth, camHeight);		//  Difference Image between Background and Source
        floatBgImg.allocate(camWidth, camHeight);	//  ofxShortImage used for simple dynamic background subtraction
        
    }
    
    void applyFilters(CPUImageFilter& img, const ofPoint *src, const ofPoint *dst){
        
        //  Set Mirroring Horizontal/Vertical
        //
        
            img.mirror(bVerticalMirror, bHorizontalMirror);
       
            grayImg = img; //for drawing
        
        //  Warp
        //
        original = img;
        img.warpIntoMe( original , src, dst);
        
        //  Dynamic background with learn rate
        //
        if(bDynamicBG){
            floatBgImg.addWeighted( img, fLearnRate);
			//grayBg = floatBgImg;  // not yet implemented
            cvConvertScale( floatBgImg.getCvImage(), grayBg.getCvImage(), 255.0f/65535.0f, 0 );
            grayBg.flagImageChanged();
        }
        
        //  Recapature the background until image/camera is fully exposed
        //
        if((ofGetElapsedTimeMillis() - exposureStartTime) < CAMERA_EXPOSURE_TIME) bLearnBakground = true;
        
        //  Capture full background
        //
        if (bLearnBakground == true){
            floatBgImg = img;
			//grayBg = floatBgImg;  // not yet implemented
			cvConvertScale( floatBgImg.getCvImage(), grayBg.getCvImage(), 255.0f/65535.0f, 0 );
			grayBg.flagImageChanged();
            bLearnBakground = false;
        }
        
		//Background Subtraction
        //img.absDiff(grayBg, img);
        
		if(bTrackDark)
			cvSub(grayBg.getCvImage(), img.getCvImage(), img.getCvImage());
		else
			cvSub(img.getCvImage(), grayBg.getCvImage(), img.getCvImage());
        
		img.flagImageChanged();
        
        //DelMatt
        
       
            grayDiff = img; //for drawing
    }
    
    void draw(){
//        grayImg.draw(30, 15, 320, 240);
//        original.draw(30, 15, 320, 240);
        //ofSetColor(255, 255, 255, 255);
       // grayDiff.draw(1920, 0, 1920, 1080);
       if(bRevealSettings)
       {
        ofSetColor(255, 255, 255, 255);
        floatBgImg.draw(2340, 620, 128, 96);
       }
       
        
    }

};
#endif
