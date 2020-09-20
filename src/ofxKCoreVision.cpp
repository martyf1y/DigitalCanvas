/*
 *  ofxKCoreVision.cpp
 *  based on ofxNCoreVision.cpp
 *  NUI Group Community Core Vision
 *
 *  Created by NUI Group Dev Team A on 2/1/09.
 *  Copyright 2009 NUI Group. All rights reserved.
 *
 */

#include "ofxKCoreVision.h"
#include "Controls/gui.h"
#include "testApp.h"



/******************************************************************************
 * The setup function is run once to perform initializations in the application
 *****************************************************************************/


void ofxKCoreVision::setup() ////VidRecording
{
    ofEventArgs dummyArgs;
    _setup(dummyArgs);
    
}
void ofxKCoreVision::RECsetup() ////VidRecording
{
    /////////VidRecording////////////
    ofEnableAlphaBlending();
    
    brushImg.loadImage("brush.png");
    
    int width = 1920;
    int height = 1080;
    
    maskFbo.allocate(width,height);
    fbo.allocate(width,height);
    
    // There are 3 of ways of loading a shader:
    //
    //  1 - Using just the name of the shader and ledding ofShader look for .frag and .vert:
    //      Ex.: shader.load( "myShader");
    //
    //  2 - Giving the right file names for each one:
    //      Ex.: shader.load( "myShader.vert","myShader.frag");
    //
    //  3 - And the third one itÔøΩs passing the shader programa on a single string;
    //
    
#ifdef TARGET_OPENGLES
    shader.load("shaders_gles/alphamask.vert","shaders_gles/alphamask.frag");
#else
    if(ofGetGLProgrammableRenderer()){
    	string vertex = "#version 150\n\
    	\n\
		uniform mat4 projectionMatrix;\n\
		uniform mat4 modelViewMatrix;\n\
    	uniform mat4 modelViewProjectionMatrix;\n\
    	\n\
    	\n\
    	in vec4  position;\n\
    	in vec2  texcoord;\n\
    	\n\
    	out vec2 texCoordVarying;\n\
    	\n\
    	void main()\n\
    	{\n\
            texCoordVarying = texcoord;\
            gl_Position = modelViewProjectionMatrix * position;\n\
    	}";
		string fragment = "#version 150\n\
		\n\
		uniform sampler2DRect tex0;\
		uniform sampler2DRect maskTex;\
        in vec2 texCoordVarying;\n\
		\
        out vec4 fragColor;\n\
		void main (void){\
            vec2 pos = texCoordVarying;\
            \
            vec3 src = texture(tex0, pos).rgb;\
            float mask = texture(maskTex, pos).r;\
            \
            fragColor = vec4( src , mask);\
		}";
		shader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
		shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
		shader.bindDefaults();
		shader.linkProgram();
    }else{
		string shaderProgram = "#version 120\n \
#extension GL_ARB_texture_rectangle : enable\n \
\
uniform sampler2DRect tex0;\
uniform sampler2DRect maskTex;\
\
void main (void){\
vec2 pos = gl_TexCoord[0].st;\
\
vec3 src = texture2DRect(tex0, pos).rgb;\
float mask = texture2DRect(maskTex, pos).r;\
\
gl_FragColor = vec4( src , mask);\
}";
shader.setupShaderFromSource(GL_FRAGMENT_SHADER, shaderProgram);
		shader.linkProgram();
    }
#endif
    
    // LetÔøΩs clear the FBOÔøΩs
    // otherwise it will bring some junk with it from the memory
    maskFbo.begin();
    ofClear(0,0,0,255);
    maskFbo.end();
    
    fbo.begin();
    ofClear(0,0,0,255);
    fbo.end();
    
    bBrushDown = false;
    
    ////////////////////Webcam///////////////
    
    webGrabber.setDeviceID(0);
    webGrabber.setDesiredFrameRate(30);
    webGrabber.initGrabber(1920, 1080); //Grab webcam dimensions
    
    ///////////////////Blending///////////////
    blendMode = OF_BLENDMODE_ALPHA;
    
}

void ofxKCoreVision::_setup(ofEventArgs &e){
    
    
    
    threshold       = 80;
	nearThreshold   = 550;
	farThreshold    = 650;
    
	//set the title
	ofSetWindowTitle("Kinect Vision based on CCV v2.1");
    
	//create filter
	if(filter == NULL)
		filter = new ProcessFilters();
    
	//Load Settings from config.xml file
	loadXMLSettings();
    
	if(debugMode){
		printf("DEBUG MODE : Printing to File\n");
		/*****************************************************************************************************
         * LOGGING
         ******************************************************************************************************/
		/* alright first we need to get time and date so our logs can be ordered */
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		strftime (fileName,80,"../logs/log_%B_%d_%y_%H_%M_%S.txt",timeinfo);
		FILE *stream;
		sprintf(fileName, ofToDataPath(fileName).c_str());
		if((stream = freopen(fileName, "w", stdout)) == NULL){}
		/******************************************************************************************************/
	}
    
	//  Setup Window Properties
    //
	//ofSetWindowShape(1300,900);
	ofSetVerticalSync(false);	            //Set vertical sync to false for better performance?
    
	//  Init Kinect Sensor Device
    //
	//  save/update log file
	if(debugMode) if( (stream = freopen(fileName, "a", stdout) ) == NULL){}
    cameraInited = false;
	if ( kinect.init(false,false,false) ){
        cameraInited    =   kinect.open();
        camWidth		=	kinect.getWidth();
        camHeight		=	kinect.getHeight();
        int camRate     =   30;
        
        ofSetFrameRate(camRate * 1.3);			//  This will be based on camera fps in the future
        printf("Kinect Initialised...\n");
    }
    
	//  Allocate images (needed for drawing/processing images)
    //
    sourceImg.allocate(camWidth, camHeight);    //  Source Image
	sourceImg.setUseTexture(false);				//  We don't need to draw this so don't create a texture
	processedImg.allocate(camWidth, camHeight); //  main Image that'll be processed.
	processedImg.setUseTexture(false);			//  We don't need to draw this so don't create a texture
    
	//  GUI Stuff
    //
	verdana.loadFont("verdana.ttf", 8, true, true);
	//background.loadimage(255, 255, 255);
	controls = ofxGui::Instance(this);
	setupControls();
    
	//  Setup Calibration
    //
	calib.setup(camWidth, camHeight, &tracker);
    
	//  Allocate Filters
    //
	filter->allocate( camWidth, camHeight );
    
	/*****************************************************************************************************
     * Startup Modes
     ******************************************************************************************************/
    
	//If Standalone Mode (not an addon)
	if (bStandaloneMode){
		printf("Starting in standalone mode...\n\n");
		showConfiguration = true;
	}
	
    bShowInterface = true;
    printf("Starting in full mode...\n\n");
	
    
	//If Object tracking activated
	if(contourFinder.bTrackObjects) {
		templates.loadTemplateXml();
	}
    
	contourFinder.setTemplateUtils(&templates);
    
	printf("Community Core Vision is setup!\n\n");
    
    RECsetup(); ////VidRecording
}

/****************************************************************
 *	Load/Save config.xml file Settings
 ****************************************************************/
bool ofxKCoreVision::loadXMLSettings(){
	message = "Loading config.xml...";
    
    ofxXmlSettings		XML;
    if ( XML.loadFile("config.xml") ){
        if ( XML.pushTag("CONFIG") ){
            
            
            
            nearThreshold				= XML.getValue("KINECT:NEAR",600);
            farThreshold				= XML.getValue("KINECT:FAR",700);
            
            XML.pushTag("WARP");
            for(int i = 0; i < 4; i++){
                XML.pushTag("POINT",i);
                srcPoints[i].x = XML.getValue("X", 0.0);
                srcPoints[i].y = XML.getValue("Y", 0.0);
                XML.popTag();
            }
            XML.popTag();
            
            maxBlobs					= XML.getValue("BLOBS:MAXNUMBER", 20);
            
            backgroundLearnRate			= XML.getValue("INT:BGLEARNRATE", 0.01f);
            
            bShowLabels					= XML.getValue("BOOLEAN:LABELS",0);
            bDrawOutlines				= XML.getValue("BOOLEAN:OUTLINES",0);
            
            //  Pre-Filter
            //
            filter->bLearnBakground		= XML.getValue("BOOLEAN:LEARNBG",0);
            filter->bVerticalMirror		= XML.getValue("BOOLEAN:VMIRROR",0);
            filter->bHorizontalMirror	= XML.getValue("BOOLEAN:HMIRROR",0);
            
            //  Filters
            //
            filter->bTrackDark			= XML.getValue("BOOLEAN:TRACKDARK", 0);
            //DelMatt
            filter->bDynamicBG			= XML.getValue("BOOLEAN:DYNAMICBG", 1);
            
            //  Filter Settings
            //
            filter->threshold			= XML.getValue("INT:THRESHOLD",0);
            //DelMatt
            
            //  CounterFinder
            //
            minTempArea					= XML.getValue("INT:MINTEMPAREA",0);
            maxTempArea					= XML.getValue("INT:MAXTEMPAREA",0);
            MIN_BLOB_SIZE				= XML.getValue("INT:MINBLOBSIZE",2);
            MAX_BLOB_SIZE				= XML.getValue("INT:MAXBLOBSIZE",100);
            hullPress					= XML.getValue("INT:HULLPRESS",20.0);
            tracker.MOVEMENT_FILTERING	= XML.getValue("INT:MINMOVEMENT",0);
            
            //  Tracking Options
            //
            contourFinder.bTrackBlobs	= XML.getValue("BOOLEAN:TRACKBLOBS",0);
            contourFinder.bTrackFingers	= XML.getValue("BOOLEAN:TRACKFINGERS",0);
            contourFinder.bTrackObjects	= XML.getValue("BOOLEAN:TRACKOBJECTS",0);
            
            //  NETWORK SETTINGS
            //
            bTUIOMode					= XML.getValue("BOOLEAN:TUIO",0);
            myTUIO.bOSCMode				= XML.getValue("BOOLEAN:OSCMODE",1);
            myTUIO.bTCPMode				= XML.getValue("BOOLEAN:TCPMODE",1);
            myTUIO.bBinaryMode			= XML.getValue("BOOLEAN:BINMODE",1);
            myTUIO.bHeightWidth			= XML.getValue("BOOLEAN:HEIGHTWIDTH",0);
            tmpLocalHost				= XML.getValue("NETWORK:LOCALHOST", "localhost");
            tmpPort						= XML.getValue("NETWORK:TUIOPORT_OUT", 3333);
            tmpFlashPort				= XML.getValue("NETWORK:TUIOFLASHPORT_OUT", 3000);
            
            XML.popTag();
            
            myTUIO.setup(tmpLocalHost.c_str(), tmpPort, tmpFlashPort); //have to convert tmpLocalHost to a const char*
            message = "Settings Loaded!\n\n";
            return true;
        } else {
            message = "The settings file was empty!\n\n";
            return false;
        }
		
	} else {
		message = "No Settings Found...\n\n"; //FAIL
        return false;
    }
}

bool ofxKCoreVision::saveXMLSettings(){
    ofxXmlSettings		XML;
    
    XML.loadFile("config.xml");
    
	XML.setValue("CONFIG:KINECT:NEAR", nearThreshold);
	XML.setValue("CONFIG:KINECT:FAR", farThreshold);
    
	XML.setValue("CONFIG:BOOLEAN:PRESSURE",bShowPressure);
	XML.setValue("CONFIG:BOOLEAN:LABELS",bShowLabels);
	XML.setValue("CONFIG:BOOLEAN:OUTLINES",bDrawOutlines);
	XML.setValue("CONFIG:BOOLEAN:LEARNBG", filter->bLearnBakground);
	XML.setValue("CONFIG:BOOLEAN:VMIRROR", filter->bVerticalMirror);
	XML.setValue("CONFIG:BOOLEAN:HMIRROR", filter->bHorizontalMirror);
	XML.setValue("CONFIG:BOOLEAN:TRACKDARK", filter->bTrackDark);
	//DelMatt
	XML.setValue("CONFIG:BOOLEAN:DYNAMICBG", filter->bDynamicBG);
	XML.setValue("CONFIG:INT:MINMOVEMENT", tracker.MOVEMENT_FILTERING);
	XML.setValue("CONFIG:INT:MINBLOBSIZE", MIN_BLOB_SIZE);
	XML.setValue("CONFIG:INT:MAXBLOBSIZE", MAX_BLOB_SIZE);
	XML.setValue("CONFIG:INT:BGLEARNRATE", backgroundLearnRate);
	XML.setValue("CONFIG:INT:THRESHOLD", filter->threshold);
    //DelMatt
	XML.setValue("CONFIG:INT:MINTEMPAREA", minTempArea);
	XML.setValue("CONFIG:INT:MAXTEMPAREA", maxTempArea);
	XML.setValue("CONFIG:INT:HULLPRESS", hullPress);
    //DelMatt
	XML.setValue("CONFIG:BOOLEAN:TUIO",bTUIOMode);
	XML.setValue("CONFIG:BOOLEAN:TRACKBLOBS",contourFinder.bTrackBlobs);
	XML.setValue("CONFIG:BOOLEAN:TRACKFINGERS",contourFinder.bTrackFingers);
	XML.setValue("CONFIG:BOOLEAN:TRACKOBJECTS",contourFinder.bTrackObjects);
	XML.setValue("CONFIG:BOOLEAN:HEIGHTWIDTH", myTUIO.bHeightWidth);
	XML.setValue("CONFIG:BOOLEAN:OSCMODE", myTUIO.bOSCMode);
	XML.setValue("CONFIG:BOOLEAN:TCPMODE", myTUIO.bTCPMode);
	XML.setValue("CONFIG:BOOLEAN:BINMODE", myTUIO.bBinaryMode);
    
    XML.pushTag("CONFIG");
    XML.pushTag("WARP");
    for(int i = 0 ; i < 4; i++){
        XML.setValue("POINT:X", srcPoints[i].x,i);
        XML.setValue("POINT:Y", srcPoints[i].y,i);
    }
    XML.popTag();
    XML.popTag();
    
    return XML.saveFile("config.xml");
}

/******************************************************************************
 * The update function runs continuously. Use it to update states and variables
 *****************************************************************************/
void ofxKCoreVision::RECupdate(){ ////VidRecording
    // MASK (frame buffer object)
    //
    ofPoint mouse = ofPoint(ofGetMouseX(),ofGetMouseY());
    maskFbo.begin();
    if(bShowMask)
    {
        ofNoFill();
        ofSetColor(255, 255, 255, 209);
        brushImg.draw(iMaskPosX - (iMaskWidth/2),iMaskPosY - (iMaskHeight/2),iMaskWidth,iMaskHeight);
        ///Setting up the recording
        if(!bRecord)
        {
            cout << "Let's start recording" << endl;
            //TRIGGER
            //ISADORA OLD
            bRecord = true;
            bRecordingTrigger = true;
            bInteracting = true;
        }
    }
    
    else if (bBrushDown)
    {
        brushImg.draw(mouse.x - 300,mouse.y - 200,600,400);
    }
    else if(!bShowMask)
    {
        if (bRecord)
        {
            
            bRecord = false;
            cout << "Let's stop recording" << endl;
            //Stop the recording variable here
            bInteracting = false;
            bRecordingTrigger = false;
        }
    }
    maskFbo.end();
    
    
    // HERE the shader-masking happends
    //
    ofRectangle webWindow(0, 0, 1920, 1080); //Creates black background
    
    fbo.begin();
    // Cleaning everthing with alpha mask on 0 in order to make it transparent for default
    ofClear(0, 0, 0, 0);
    
    shader.begin();
    shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1 );
    //Webcam has to be inside fbo
    ofSetHexColor(0xffffff);
    ofRectangle webGrabberRect(0,0,1920,1080); //This is on big screen
    webGrabberRect.scaleTo(webWindow);
    if (bBrushDown || bShowMask){ // I put this in to hopefully speed things up a little ,
        //well you just wasted 10 minutes of my time putting that in matt
        webGrabber.draw(webGrabberRect);
    }
    shader.end();
    fbo.end();
    
    //Webcam
    
    webGrabber.update();
    
    //Matt this updates the mask so only its position is revealed
    maskFbo.begin(); //Turn this off to see a cool spray paint effect
    ofClear(0,0,0,255);
    maskFbo.end();
}

void ofxKCoreVision::_update(ofEventArgs &e){
    
    // if (!cameraInited) return;
    
	if(debugMode) if((stream = freopen(fileName, "a", stdout)) == NULL){}
    
    //  Dragable Warp points
    //
    if ( !bCalibration && bShowInterface && ofGetMousePressed() ){
        ofPoint mouse = ofPoint(ofGetMouseX(),ofGetMouseY());
        mouse -= ofPoint(420, 620);   //  Translate
        mouse *= 2.0;               //  Scale
        
        if ( pointSelected == -1 ){
            for(int i = 0; i < 4; i++){
                if ( srcPoints[i].distance(mouse) < 20){
                    srcPoints[i].x = mouse.x;
                    srcPoints[i].y = mouse.y;
                    pointSelected = i;
                    break;
                }
            }
        } else {
            if (mouse.x < 0)
                mouse.x = 0;
            
            if (mouse.x > camWidth)
                mouse.x = camWidth;
            
            if (mouse.y < 0)
                mouse.y = 0;
            
            if (mouse.y > camHeight)
                mouse.y = camHeight;
            
            srcPoints[pointSelected].x = mouse.x;
            srcPoints[pointSelected].y = mouse.y;
        }
    } else {
        pointSelected = -1;
    }
    
    kinect.update();
	bNewFrame = true;
	if(!bNewFrame){
		return;			//if no new frame, return
	} else {			//else process camera frame
		ofBackground(0, 0, 0);
        
		// Calculate FPS of Camera
        //
		frames++;
		float time = ofGetElapsedTimeMillis();
		if (time > (lastFPSlog + 1000)) {
			fps = frames;
			frames = 0;
			lastFPSlog = time;
		}
        // End calculation
        
		float beforeTime = ofGetElapsedTimeMillis();
        
        //  Get Pixels from the Kinect
        //
        if(kinect.isFrameNew()) {
            
            const float* depthRaw = kinect.getDistancePixels();
            unsigned char * depthPixels = sourceImg.getPixels();
            
            int numPixels = camWidth * camHeight;
            
            for(int i = 0; i < numPixels; i++, depthRaw++) {
                if((*depthRaw <= farThreshold) && (*depthRaw >= nearThreshold))
                    depthPixels[i] = ofMap(*depthRaw, nearThreshold, farThreshold, 255,0);
                else
                    depthPixels[i] = 0;
            }
            
            sourceImg.flagImageChanged();
        }
        processedImg = sourceImg;
        
        //  Filter and Process
        //
        filter->applyFilters( processedImg, srcPoints, dstPoints );
        contourFinder.findContours(processedImg,  (MIN_BLOB_SIZE * 2) + 1, ((camWidth * camHeight) * .4) * (MAX_BLOB_SIZE * .001), maxBlobs, (double) hullPress, false);
        
		//  If Object tracking or Finger tracking is enabled
		//
        if( contourFinder.bTrackBlobs || contourFinder.bTrackFingers || contourFinder.bTrackObjects){
			tracker.track(&contourFinder);
        }
        
        //  Get DSP time
        //
        differenceTime = ofGetElapsedTimeMillis() - beforeTime;
        
        //Dynamic Background subtraction LearRate
        //
        if (filter->bDynamicBG){
            filter->fLearnRate = backgroundLearnRate * .0001; //If there are no blobs, add the background faster.
			
            if ((contourFinder.nBlobs > 0 )|| (contourFinder.nFingers > 0 ) ) //If there ARE blobs, add the background slower.
				filter->fLearnRate = backgroundLearnRate * .0001;
        }
        
		//Sending TUIO messages
        //
		if (myTUIO.bOSCMode || myTUIO.bTCPMode || myTUIO.bBinaryMode){
			myTUIO.setMode(contourFinder.bTrackBlobs, contourFinder.bTrackFingers , contourFinder.bTrackObjects);
			myTUIO.sendTUIO(tracker.getTrackedBlobsPtr(), tracker.getTrackedFingersPtr(), tracker.getTrackedObjectsPtr() );
		}
	}
    RECupdate();////VidRecording
}

void ofxKCoreVision::RECdraw()
{
    ofSetColor(255,255);
    
    //Blending video and webcam together
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    
    //Blending mask with video
    ofEnableBlendMode(blendMode);
    fbo.draw(0,0);
    ofDisableBlendMode();
    
    if(bShowMask)
    {
        
        //This is to make sure the rectangle does not go onto the other screen.
        if((iMaskPosX - iMaskWidth/2) > 2)
        {
            iRectPosx = 1920 + iMaskPosX - (iMaskWidth/2);
            iRectWidth = iMaskWidth;
        }
        else if((iMaskPosX - iMaskWidth/2) < 1)
        {
            iRectWidth = iMaskWidth + (iMaskPosX - iMaskWidth/2);
        }
        
        iRectPosy = iMaskPosY - (iMaskHeight/2);
        iRectHeight = iMaskHeight;
        
        
        ofPushStyle();
        ofNoFill();
        ofSetLineWidth(4);
        //make a nice flashy red record color
        ofSetColor(255, 255, 255, 240);
        ofRect(iRectPosx,iRectPosy,iRectWidth,iRectHeight);
        ofPopStyle();
        
    }
    ofDrawBitmapString("Rectangle HxW: " + ofToString(iRectWidth) + " " + ofToString(iRectHeight), 1960, 170);
    ofDrawBitmapString("Show the mask?: "  + ofToString(bShowMask?"Yes":"No"), 1960, 190);

}

void ofxKCoreVision::_draw(ofEventArgs &e){
	if (showConfiguration){
        
        //  Check the mode
        //
		if (bCalibration){
			//  Don't draw main interface
			calib.passInContourFinder(contourFinder.nBlobs, contourFinder.blobs);
			calib.doCalibration();
		} else if (bShowInterface){
			drawFullMode();
            
			if(bDrawOutlines || bShowLabels)
				drawOutlines();
            
			if(contourFinder.bTrackObjects && isSelecting){
				ofNoFill();
				ofSetColor(255, 0, 0);
				ofRect(rect.x,rect.y,rect.width,rect.height);
				ofSetColor(0, 255, 0);
				ofRect(minRect.x,minRect.y,minRect.width, minRect.height);
				ofSetColor(0, 0, 255);
				ofRect(maxRect.x, maxRect.y, maxRect.width, maxRect.height);
			}
		}
		//  Draw gui controls
        //
        
        if (!bCalibration && bRevealSettings)
            controls->draw();
        
	}
    RECdraw();
}

void ofxKCoreVision::drawFullMode(){
	ofSetColor(255);
    
	//  Draw Background Image
    //
	
    
    //  Draw Images
    //
    filter->draw();
    
    
    if(bRevealSettings)
    {
        //  Draw Warp Area
        //
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(iDetectorVisualX, iDetectorVisualY);
        ofScale(0.5, 0.5);
        for(int i = 0; i < 4; i++){
            ofSetColor(255,0,0,100);
            
            if (i == pointSelected)
                ofFill();
            else
                ofNoFill();
            
            ofRect(srcPoints[i].x-7, srcPoints[i].y-7,14,14);
            ofLine(srcPoints[i], srcPoints[(i+1)%4]);
        }
        ofPopMatrix();
        ofPopStyle();
        
        ofFill();
        ofSetColor(151, 155, 183);
        ofRect(2340, 720, 128, 120);
        //  Draw Info panel
        // Here it counts the blobs and finger MATT
        string str0 = "FPS: ";
        str0+= ofToString(fps, 0)+"\n";
        string str1 = "Resolution: ";
        str1+= ofToString(camWidth, 0) + "x" + ofToString(camHeight, 0)  + "\n";
        string str2 = "Blobs: ";
        str2+= ofToString(contourFinder.nBlobs,0)+", "+ofToString(contourFinder.nObjects,0)+"\n";
        string str3 = "Fingers: ";
        str3+= ofToString(contourFinder.nFingers,0)+"\n";
        ofSetColor(0);
        //PosChange
        verdana.drawString( str0 + str1 + str2 + str3, 2340, 731);
        
        //  Draw TUIO data
        //
        char buf[256]="";
        if(myTUIO.bOSCMode && myTUIO.bTCPMode)
            sprintf(buf, "Dual Mode");
        else if(myTUIO.bOSCMode)
            sprintf(buf, "Host: %s\nProtocol: UDP [OSC]\nPort: %i", myTUIO.localHost, myTUIO.TUIOPort);
        else if(myTUIO.bTCPMode){
            if(myTUIO.bIsConnected)
                sprintf(buf, "Host: %s\nProtocol: TCP [XML]\nPort: %i", myTUIO.localHost, myTUIO.TUIOFlashPort);
            else
                sprintf(buf, "Binding Error\nHost: %s\nProtocol: TCP [XML]\nPort: %i", myTUIO.localHost, myTUIO.TUIOFlashPort);
        } else if(myTUIO.bBinaryMode){
            if(myTUIO.bIsConnected)
                sprintf(buf, "Host: %s\nProtocol: Binary\nPort: %i", myTUIO.localHost, myTUIO.TUIOFlashPort);
            else
                sprintf(buf, "Binding Error\nHost: %s\nProtocol: Binary\nPort: %i", myTUIO.localHost, myTUIO.TUIOFlashPort);
        }
        ofSetColor(0);
        //PosChange
        verdana.drawString(buf, 2340, 786);
    }
}

void ofxKCoreVision::drawOutlines(){
    ofPushStyle();
    
	//  Find the blobs for drawing
    //
	if(contourFinder.bTrackBlobs){
        
		for (int i=0; i<contourFinder.nBlobs; i++){
            
            fBlob1xpos = contourFinder.blobs[contourFinder.nBlobs-1].centroid.x * (MAIN_WINDOW_WIDTH/camWidth);
            fBlob1ypos = contourFinder.blobs[contourFinder.nBlobs-1].centroid.y * (MAIN_WINDOW_HEIGHT/camHeight);
            fBlob2xpos = contourFinder.blobs[contourFinder.nBlobs-2].centroid.x * (MAIN_WINDOW_WIDTH/camWidth);
            fBlob2ypos = contourFinder.blobs[contourFinder.nBlobs-2].centroid.y * (MAIN_WINDOW_HEIGHT/camHeight);
            
            //This is to make sure that whatever is on the left is the right left corner variable and so on...
            if(fBlob1xpos > fBlob2xpos)
            {
                fTopLeftCornerx = fBlob2xpos;
                fBottomRightCornerx = fBlob1xpos;
            }
            else
            {
                fTopLeftCornerx = fBlob1xpos;
                fBottomRightCornerx = fBlob2xpos;
            }
            //Make the y values correct too
            if(fBlob1ypos > fBlob2ypos)
            {
                fTopLeftCornery = fBlob2ypos;
                fBottomRightCornery = fBlob1ypos;
            }
            else
            {
                fTopLeftCornery = fBlob1ypos;
                fBottomRightCornery = fBlob2ypos;
            }
            
            ////////////////////MATT/////////////////////////
            
            //PosChange
			if (bDrawOutlines) //Draw contours (outlines) on the source image
				contourFinder.blobs[i].drawContours(iDetectorVisualX, iDetectorVisualY, camWidth, camHeight, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
            
            
            if (bShowLabels && bRevealSettings){ //Show ID label
                float xpos = contourFinder.blobs[i].centroid.x * (MAIN_WINDOW_WIDTH/camWidth);
                float ypos = contourFinder.blobs[i].centroid.y * (MAIN_WINDOW_HEIGHT/camHeight);
                
                ofSetColor(200,255,200);
                char idStr[1024];
                
                sprintf(idStr, "id: %i", contourFinder.blobs[i].id);
                verdana.drawString(idStr, xpos + 1920, ypos + contourFinder.blobs[i].boundingRect.height/2);
                
            }
            
		}
        
        ////Matt////
        ///Choosing masking type///
        
        //MATT YOU STILL NEED TO FIX GLITCHY HANDS
        if (contourFinder.nBlobs == 0)
        {
            
            if(iMaskTimerLocation!=0) //If it doesn't == start timing
            {
                //  cout << "00000000" << endl;
                iMaskPassedTime = ofGetElapsedTimeMillis() - iMaskSavedTime;
            }
            else if(iMaskTimerLocation==0)//If it does work keep working
            {
                bShowMask = false;
                iMaskSavedTime = ofGetElapsedTimeMillis();
                iMaskPassedTime = ofGetElapsedTimeMillis() - iMaskSavedTime;
            }
            if(iMaskPassedTime > iMaskTotalTime) //If timer finishes make it work
            {
                iMaskTimerLocation = 0;
            }
            
        }
        if(contourFinder.nBlobs < 2 && contourFinder.nBlobs > 0)
        {
            if(iMaskTimerLocation!=1) //This to to stop any random glitching
            {
                iMaskPassedTime = ofGetElapsedTimeMillis() - iMaskSavedTime;
            }
            else if(iMaskTimerLocation==1)
            {
                bShowMask = true;
                if(iMaskWidth < 75)
                {
                    iMaskWidth = 75;
                }
                if(iMaskHeight < 75)
                {
                    iMaskHeight = 75;
                }
                iMaskPosX = fBottomRightCornerx;
                iMaskPosY = fBottomRightCornery;
                iMaskSavedTime = ofGetElapsedTimeMillis();
                iMaskPassedTime = ofGetElapsedTimeMillis() - iMaskSavedTime;
            }
            
            if(iMaskPassedTime > iMaskTotalTime)
            {
                iMaskTimerLocation = 1;
            }
        }
        else if(contourFinder.nBlobs >= 2)
        {
            if(iMaskTimerLocation!=2) //This to to stop any random glitching
            {
                iMaskPassedTime = ofGetElapsedTimeMillis() - iMaskSavedTime;
            }
            else if(iMaskTimerLocation==2)
            {
                bShowMask = true;
                iMaskWidth =  fBottomRightCornerx - fTopLeftCornerx; //doubled for screen size +1 because of image dimensions
                iMaskHeight = fBottomRightCornery - fTopLeftCornery;
                iMaskPosX = (fTopLeftCornerx + fBottomRightCornerx)/2;
                iMaskPosY = (fTopLeftCornery + fBottomRightCornery)/2;
                iMaskSavedTime = ofGetElapsedTimeMillis();
                iMaskPassedTime = ofGetElapsedTimeMillis() - iMaskSavedTime;
            }
            
            if(iMaskPassedTime > iMaskTotalTime)
            {
                iMaskTimerLocation = 2;
            }
            
        }
        
        
	}
    
	//  Find the fingers for drawing
    //
    
	if(contourFinder.bTrackFingers){
		for (int i=0; i<contourFinder.nFingers; i++) {
            //PosChange
			if (bDrawOutlines) //Draw contours (outlines) on the source image
				contourFinder.fingers[i].drawCenter(iDetectorVisualX, iDetectorVisualY, camWidth, camHeight, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
            
            if (bShowLabels && bRevealSettings){ //Show ID label
                float xpos = contourFinder.fingers[i].centroid.x * (MAIN_WINDOW_WIDTH/camWidth);
                float ypos = contourFinder.fingers[i].centroid.y * (MAIN_WINDOW_HEIGHT/camHeight);
                //The co ordinates of the fingers colour MATT
                ofSetColor(255,255,0);
                char idStr[1024];
                
                sprintf(idStr, "id: %i", contourFinder.fingers[i].id);
                
                verdana.drawString(idStr, xpos + 1920, ypos + contourFinder.fingers[i].boundingRect.height/2 - 120);
            }
            
        }
	}
    
    
	//  Object Drawing
    //
	if(contourFinder.bTrackObjects){
		for (int i=0; i<contourFinder.nObjects; i++){
            //PosChange
			if (bDrawOutlines) //Draw contours (outlines) on the source image
				contourFinder.objects[i].drawBox(iDetectorVisualX, iDetectorVisualY, camWidth, camHeight, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
            
            if (bShowLabels && bRevealSettings){ //Show ID label
                float xpos = contourFinder.objects[i].centroid.x * (MAIN_WINDOW_WIDTH/camWidth);
                float ypos = contourFinder.objects[i].centroid.y * (MAIN_WINDOW_HEIGHT/camHeight);
                
                ofSetColor(200,255,200);
                char idStr[1024];
                
                sprintf(idStr, "id: %i", contourFinder.objects[i].id);
                
                verdana.drawString(idStr, xpos + 20, ypos + contourFinder.objects[i].boundingRect.height/2 + 45);
            }
            
            
            
            
            
        }
        
    }
    
	ofPopStyle();
}

/*****************************************************************************
 * KEY EVENTS
 *****************************************************************************/


void ofxKCoreVision::_keyPressed(ofKeyEventArgs &e)
{
	if (showConfiguration){
		switch (e.key){
            case 'b':
                filter->bLearnBakground = true;
                break;
            case 'o':
                bDrawOutlines ? bDrawOutlines = false : bDrawOutlines = true;
                controls->update(appPtr->trackedPanel_outlines, kofxGui_Set_Bool, &appPtr->bDrawOutlines, sizeof(bool));
                break;
            case 'h':
                filter->bHorizontalMirror ? filter->bHorizontalMirror = false : filter->bHorizontalMirror = true;
                controls->update(appPtr->propertiesPanel_flipH, kofxGui_Set_Bool, &appPtr->filter->bHorizontalMirror, sizeof(bool));
                break;
            case 'j':
                filter->bVerticalMirror ? filter->bVerticalMirror = false : filter->bVerticalMirror = true;
                controls->update(appPtr->propertiesPanel_flipV, kofxGui_Set_Bool, &appPtr->filter->bVerticalMirror, sizeof(bool));
                break;
            case 't':
                myTUIO.bOSCMode = !myTUIO.bOSCMode;
                myTUIO.bTCPMode = false;
                myTUIO.bBinaryMode = false;
                bTUIOMode = myTUIO.bOSCMode;
                controls->update(appPtr->optionPanel_tuio_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bTCPMode, sizeof(bool));
                controls->update(appPtr->optionPanel_tuio_osc, kofxGui_Set_Bool, &appPtr->myTUIO.bOSCMode, sizeof(bool));
                controls->update(appPtr->optionPanel_bin_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bBinaryMode, sizeof(bool));
                //clear blobs
                //			myTUIO.blobs.clear();
                //			myTUIO.fingers.clear();
                //			myTUIO.objects.clear();
                break;
            case 'f':
                myTUIO.bOSCMode = false;
                myTUIO.bTCPMode = !myTUIO.bTCPMode;
                myTUIO.bBinaryMode = false;
                bTUIOMode = myTUIO.bTCPMode;
                controls->update(appPtr->optionPanel_tuio_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bTCPMode, sizeof(bool));
                controls->update(appPtr->optionPanel_tuio_osc, kofxGui_Set_Bool, &appPtr->myTUIO.bOSCMode, sizeof(bool));
                controls->update(appPtr->optionPanel_bin_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bBinaryMode, sizeof(bool));
                //clear blobs
                //			myTUIO.blobs.clear();
                //			myTUIO.fingers.clear();
                //			myTUIO.objects.clear();
                break;
            case 'n':
                myTUIO.bOSCMode = false;
                myTUIO.bTCPMode = false;
                myTUIO.bBinaryMode = !myTUIO.bBinaryMode;
                bTUIOMode = myTUIO.bBinaryMode;
                controls->update(appPtr->optionPanel_tuio_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bTCPMode, sizeof(bool));
                controls->update(appPtr->optionPanel_tuio_osc, kofxGui_Set_Bool, &appPtr->myTUIO.bOSCMode, sizeof(bool));
                controls->update(appPtr->optionPanel_bin_tcp, kofxGui_Set_Bool, &appPtr->myTUIO.bBinaryMode, sizeof(bool));
                //clear blobs
                //			myTUIO.blobs.clear();
                //			myTUIO.fingers.clear();
                //			myTUIO.objects.clear();
                break;
            case 'l':
                bShowLabels ? bShowLabels = false : bShowLabels = true;
                controls->update(appPtr->trackedPanel_ids, kofxGui_Set_Bool, &appPtr->bShowLabels, sizeof(bool));
                break;
            case 'p':
                bShowPressure ? bShowPressure = false : bShowPressure = true;
                break;
            case 'x': //Exit Calibrating
                if (bCalibration){
                    bShowInterface = true;
                    bCalibration = false;
                    calib.calibrating = false;
                    tracker.isCalibrating = false;
                    if (bFullscreen == true)
                        ofToggleFullscreen();
                    bFullscreen = false;
                }
                break;
            case OF_KEY_RETURN: //Close Template Selection and save it
                if( contourFinder.bTrackObjects && isSelecting ){
                    isSelecting = false;
                    templates.addTemplate(rect,minRect,maxRect,camWidth/320,camHeight/240);
                    rect = ofRectangle();
                    minRect = rect;
                    maxRect = rect;
                    minTempArea = 0;
                    maxTempArea = 0;
                    controls->update(appPtr->TemplatePanel_minArea, kofxGui_Set_Float, &appPtr->minTempArea, sizeof(float));
                    controls->update(appPtr->TemplatePanel_maxArea, kofxGui_Set_Float, &appPtr->maxTempArea, sizeof(float));
                }
                break;
            case OF_KEY_UP:
                farThreshold++;
                break;
            case OF_KEY_DOWN:
                farThreshold--;
                break;
            case OF_KEY_RIGHT:
                nearThreshold++;
                break;
            case OF_KEY_LEFT:
                nearThreshold--;
                break;
            default:
                break;
		}
	}
    
    //Webgrabber blending
    switch (e.key) {
        case 49:
            blendMode = OF_BLENDMODE_ALPHA;
            
            break;
        case 50:
            blendMode = OF_BLENDMODE_ADD;
        default:
            break;
    }
}

void ofxKCoreVision::_keyReleased(ofKeyEventArgs &e){
	if (showConfiguration){
		if ( e.key == 'q' && !bCalibration){
			bShowInterface = false;
			// Enter/Exit Calibration
			bCalibration = true;
			calib.calibrating = true;
			tracker.isCalibrating = true;
			if (bFullscreen == false) ofToggleFullscreen();
			bFullscreen = true;
		}
	}
	if ( e.key == '~' || e.key == '`' && !bCalibration) showConfiguration = !showConfiguration;
}


void ofxKCoreVision::_mouseDragged(ofMouseEventArgs &e){
	if (showConfiguration && bRevealSettings)
		controls->mouseDragged(e.x, e.y, e.button); //guilistener
    
    if(contourFinder.bTrackObjects){
		if( e.x > 710 && e.x < 1030 && e.y > 525 && e.y < 765 ){
			if( e.x < rect.x || e.y < rect.y ){
				rect.width = rect.x - e.x;
				rect.height = rect.y - e.y;
				rect.x = e.x;
				rect.y =  e.y;
			} else {
				rect.width = e.x - rect.x;
				rect.height = e.y - rect.y;
			}
		}
	}
}

void ofxKCoreVision::_mousePressed(ofMouseEventArgs &e)
{
	if (showConfiguration)
	{
		controls->mousePressed( e.x, e.y, e.button ); //guilistener
		if ( contourFinder.bTrackObjects )
		{
			if ( e.x > 710 && e.x < 1030 && e.y > 525 && e.y < 765 )
			{
				isSelecting = true;
				rect.x = e.x;
				rect.y = e.y;
				rect.width = 0;
				rect.height = 0;
			}
		}
		
	}
    
    //VidRecording
    //  bBrushDown = true;
}

void ofxKCoreVision::_mouseReleased(ofMouseEventArgs &e)
{
	if (showConfiguration)
		controls->mouseReleased(e.x, e.y, 0); //guilistener
    
	if( e.x > 710 && e.x < 1030 && e.y > 525 && e.y < 765 ){
		if	( contourFinder.bTrackObjects && isSelecting ){
			minRect = rect;
			maxRect = rect;
		}
	}
    
    //VidRecording
    bBrushDown = false;
}

void ofxKCoreVision::_exit(ofEventArgs &e){
	kinect.close();
    
	//  Save Settings
	saveXMLSettings();
    if(contourFinder.bTrackObjects)
		templates.saveTemplateXml();
    
	delete filter;
    filter = NULL;
	
    webGrabber.close();
    
    printf("Vision module has exited!\n");
}

