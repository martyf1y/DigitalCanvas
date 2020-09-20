#include "testApp.h"
#include "stdio.h"
#include "ofUtils.h"
#include "global.h"
//--------------------------------------------------------------
//CURRENT ISSUES
//1. Freezing from ofvideoplayer closing or loading movie, can't handle this many times or large data(?)
//2. One hand not displaying image of video, seems to 'lose' it. Looks to be a blob detection error
//3. Delay between live feed and rectangle tracker
//4. One hand detection sometimes creates errors for position, doesn't move past a certain point
void testApp::setup()
{
    //ofSetDataPathRoot("data/");
    cout << "starting Core Vision" << endl;
    ckv = new ofxKCoreVision(debug);
    // explicitely call setup, otherwise prog will crash
    ckv->setup();
    ofEnableAlphaBlending();
    ofEnableSmoothing();
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    /////////////New recorder////////////////////
    /////////////////////////////////////////////
    // 1. Create a new recorder object.  ofPtr will manage this
    // pointer for us, so no need to delete later.
    fakeRecorder = ofPtr<ofQTKitGrabber>( new ofQTKitGrabber() );
    // 2. Set our video grabber to use this source.
    vidGrabber.setGrabber(fakeRecorder);
    // 3. Make lists of our audio and video devices.
    videoDevices = fakeRecorder->listVideoDevices();
    audioDevices = fakeRecorder->listAudioDevices();
    // 3a. Optionally add audio to the recording stream.
    fakeRecorder->setAudioDeviceID(0);
    fakeRecorder->setUseAudio(true);
    
    // 4. Register for events so we'll know when videos finish saving.
    ofAddListener(fakeRecorder->videoSavedEvent, this, &testApp::videoSaved);
    
    // 4a.  If you would like to list available video codecs on your system,
    // uncomment the following code.
    vector<string> videoCodecs = fakeRecorder->listVideoCodecs();
    for(size_t i = 0; i < videoCodecs.size(); i++){
        ofLogVerbose("Available Video Codecs") << videoCodecs[i];
    }
    
    // 4b. You can set a custom / non-default codec in the following ways if desired.
    fakeRecorder->setVideoCodec("QTCompressionOptionsJPEGVideo");
    //fakeRecorder->setVideoCodec(videoCodecs[2]);
    fakeRecorder->setVideoDeviceID(2);
    // 5. Initialize the grabber.
    vidGrabber.initGrabber(1920, 1080); //Grab webcam dimensions
    
    // 6. Initialize recording on the grabber.  Call initRecording()
    // once after you've initialized the grabber.
    fakeRecorder->initRecording();
    
    vidGrabber.setDeviceID(1);
    //vidGrabber.setDesiredFrameRate(30);
    
    //we can now get back a list of devices.
    vector<ofVideoDevice> devices = vidGrabber.listDevices();
   // cout << "DEV " << devices << endl;
    for(int i = 0; i < devices.size(); i++){
        cout << devices[i].id << ": " << devices[i].deviceName;
        if( devices[i].bAvailable ){
            cout << endl;
        }else{
            cout << " - unavailable " << endl;
        }
    }
    
    bRecording = false;
    
    // If desired, you can disable the preview video.  This can help speed up recording and remove recording glitches.
    //   fakeRecorder->initGrabberWithoutPreview(); 
}

void testApp::exit()
{
    if(fakeRecorder->isRecording())
    {
        fakeRecorder->stopRecording(); //Safety safety
    }
    vidGrabber.close();
    if(MemoryPlayback.isLoaded())
    {
        MemoryPlayback.close();
    }
}

//The interaction method is to know when the person is interacting with it
void testApp::interaction()
{
    iInteractionTimer = ofGetElapsedTimeMillis() - iStartTimer; //Records interaction time
    iVidTimer = iInteractionTimer; //As of first interaction, video is the same amount of time
    //Once interaction has stopped
    if (!bInteracting)
    {
        if(iInteractionTimer > 3000)//Checks to see if performer contributed enough time
        {
            bIncludeVid = true;//Interaction sufficed, video will be recorded
        }
        else
        {
            //Because we never swap video playback, this video will be recorded overtop again since it's a wasted one.
            if(fakeRecorder->isRecording())
            { 
                fakeRecorder->stopRecording();
            }
            //This recording has been discarded, allow recording to start again
            iInteractionTimer = 0;
            iVidTimer = 0;
            bVidRecording = false;
        }
    }
}

//This will make sure the video saves when it reaches the right time and without interaction still happening
void testApp::saveVideo()
{
    iVidTimer =  ofGetElapsedTimeMillis() - iStartTimer; //start counting videotime
    if(iVidTimer >= iTotalVidTime - 50 && !bInteracting)//Checking if video is ready to be created
    {
        iTotalVidTime = iVidTimer;
        if(fakeRecorder->isRecording())
        {
            fakeRecorder->stopRecording();
        }
        MemoryPlayback.close();
        bIncludeVid = false;
        bVidRecording = false;
    }
}

//--------------------------------------------------------------
void testApp::update(){
    //Webcam
    vidGrabber.update();
    
    //Video
    if(MemoryPlayback.isLoaded())
    {
        MemoryPlayback.update();
    }
    
    //A simple video play counter
    if((MemoryPlayback.getPosition()*100) > 50 && !bUpperHalf)
    {
        bUpperHalf = true;
    }
    else if((MemoryPlayback.getPosition()*100) < 50 && bUpperHalf)
    {
        bUpperHalf = false;
        iVidPlayCount ++;
    }
    
    if(bRecordingTrigger && !bVidRecording)//checks to see if the video is still recording
    {
        iStartTimer = ofGetElapsedTimeMillis(); //Start the video length timer
        bRecording = !bRecording;
        if(bRecording)
        {
            if(b0Record)
            {
                fakeRecorder->startRecording("Experience0.mov"); //Starts the recording
            }
            else
            {
                fakeRecorder->startRecording("Experience1.mov");
            }
            bVidRecording = true;
        }
        bRecordingTrigger = false;
    }
    
    if(bVidRecording && !bIncludeVid)
    {
        interaction();
    }
    
    if(bIncludeVid)
    {
        saveVideo();
    }
}

//--------------------------------------------------------------
void testApp::draw(){
    
    ofBackground(82,89,135);
    ofFill();
    // ofRect(20, 20, 300, 300);
    ofSetColor(255,255);
    //Video
    //HAS BEEN MOVED MemoryPlayback.draw(0,0);
    ////////////////////Recording//////////////////
    
    ofRectangle playbackWindow(0, 0, 1920, 1080); //Big screen
    ofRectangle previewWindow(1920, 0, 1920, 1080);
    ofRectangle recordWindow(1922, 2, 1916, 1076);
    
    // draw the background boxes
    ofPushStyle();
    ofSetColor(0);
    ofFill();
    ofRect(previewWindow);
    ofRect(playbackWindow);
    ofPopStyle();
    
    // draw the preview if available
    if(fakeRecorder->hasPreview()){
        ofPushStyle();
        ofFill();
        ofSetColor(255);
        // fit it into the preview window, but use the correct aspect ratio
        ofRectangle videoGrabberRect(0,0,vidGrabber.getWidth(),vidGrabber.getHeight());
        //This just shows the preview of what is to be recorded, will be taken out after
        videoGrabberRect.scaleTo(ofRectangle (1920, 0, 1920, 1080)); //BIG SCREEN STUFF
        vidGrabber.draw(videoGrabberRect);
        ofPopStyle();
    } else{
        ofPushStyle();
        // x out to show there is no video preview
        ofSetColor(255);
        ofSetLineWidth(3);
        ofLine(20, 20, 640+20, 480+20);
        ofLine(20+640, 20, 20, 480+20);
        ofPopStyle();
    }
    
    // draw the playback video
    if(MemoryPlayback.isLoaded()){
        ofPushStyle();
        ofFill();
        ofSetColor(255);
        // fit it into the preview window, but use the correct aspect ratio
        ofRectangle recordedRect(ofRectangle(0,0,MemoryPlayback.getWidth(),MemoryPlayback.getHeight()));
        recordedRect.scaleTo(ofRectangle (0, 0, 1920, 1080));
        MemoryPlayback.draw(recordedRect);
        ofPopStyle();
    }
    
    ofPushStyle();
    ofNoFill();
    ofSetLineWidth(4);
    if(bRecording){
        //make a nice flashy red record color
        int flashRed = powf(1 - (sin(ofGetElapsedTimef()*10)*.5+.5),2)*255;
        ofSetColor(255, 255-flashRed, 255-flashRed);
    }
    else{
        ofSetColor(255,80);
    }
    ofRect(recordWindow);
    ofPopStyle();
    if(bRevealSettings)
    {
        //draw instructions
        ofPushStyle();
        ofSetColor(255);
        ofDrawBitmapString("' ' space bar to toggle recording", 2920, 450);
        ofDrawBitmapString("'v' switches video device", 2920, 470);
        ofDrawBitmapString("'a' switches audio device", 2920, 490);
        
        //draw video device selection
        ofDrawBitmapString("VIDEO DEVICE", 2590, 450);
        for(int i = 0; i < videoDevices.size(); i++){
            if(i == fakeRecorder->getVideoDeviceID()){
                ofSetColor(255, 100, 100);
            }
            else{
                ofSetColor(255);
            }
            ofDrawBitmapString(videoDevices[i], 2590, 470+i*20);
        }
        
        //draw audio device;
        int startY = 470+20*videoDevices.size();
        ofDrawBitmapString("AUDIO DEVICE", 2590, startY);
        startY += 20;
        for(int i = 0; i < audioDevices.size(); i++){
            if(i == fakeRecorder->getAudioDeviceID()){
                ofSetColor(255, 100, 100);
            }
            else{
                ofSetColor(255);
            }
            ofDrawBitmapString(audioDevices[i], 2590, startY+i*20);
        }
        ofPopStyle();
        ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()), 1960, 490);
        ofDrawBitmapString((bRecording?"Pause":"Start") + ofToString(" recording: r"), 1960, 510);
        ofDrawBitmapString((bRecording?"Close current video file: c":""), 1960, 530);
    }
    ofDrawBitmapString("Total Videos played: " + ofToString(iVidPlayCount), 1960, 50);
    ofDrawBitmapString("Total time played: " + ofToString(ofGetElapsedTimeMillis()/1000), 1960, 70);
    ofDrawBitmapString("Total interaction time: " + ofToString(iInteractionTimer), 1960, 90);
    ofDrawBitmapString("Current vid time: " + ofToString(iVidTimer), 1960, 110);
    ofDrawBitmapString("Largest video time: " + ofToString(iTotalVidTime), 1960, 130);
    ofDrawBitmapString("Are you interacting: " + ofToString(bInteracting?"Yes":"No"), 1960, 150);
}

void testApp::videoSaved(ofVideoSavedEventArgs& e){
    // the ofQTKitGrabber sends a message with the file name and any errors when the video is done recording
    //When recording the video has finished
    cout << "STOP RECORDING" << endl;
    bRecording = false; //Stops the chance to record, I think
    
    if(e.error.empty()){
        
        if(!MemoryPlayback.isLoaded()){
            if(b0Record)
            {
                MemoryPlayback.loadMovie("Experience0.mov"); //We load new recorder
                MemoryPlayback.play(); //We play new recorder
                b0Record = false; //Swap videos
            }
            else if(!b0Record)
            {
                MemoryPlayback.loadMovie("Experience1.mov"); //We load new recorder
                MemoryPlayback.play(); //We play new recorder
                b0Record = true; //Swap videos
            }
        }
    }
    else {
        ofLogError("videoSavedEvent") << "Video save error: " << e.error;
    }
}

//--------------------------------------------------------------
void testApp::keyPressed  (int key){
    ////////////Blending///////////////
}

//--------------------------------------------------------------
void testApp::keyReleased  (int key){
    //////////Webcam Input//////////////
    
    if(key=='r'){
        bRecording = !bRecording;
        if(bRecording) {
            fakeRecorder->startRecording("FirstExperience.mov");
        }
        bRecording = !bRecording;
        if(bRecording) {
            fakeRecorder->startRecording("SecondExperience.mov");
        }
    }
    
    if(key=='c'){
        //When recording the video has finished
        if(fakeRecorder->isRecording()){
            fakeRecorder->stopRecording();
            //MemoryPlayback.close(); //We close previous video
        }
        bRecording = false; //Stops the chance to record, I think
        
    }
    
    if(key == 'v'){
        fakeRecorder->setVideoDeviceID( (fakeRecorder->getVideoDeviceID()+1) % videoDevices.size() );
        vector<string> videoCodecs = fakeRecorder->listVideoCodecs();
        for(size_t i = 0; i < videoCodecs.size(); i++){
            ofLogVerbose("Available Video Codecs") << videoCodecs[i];
        }
        
        fakeRecorder->setVideoCodec("QTCompressionOptionsHD720SizeH264Video");
        //fakeRecorder->setVideoCodec(videoCodecs[2]);
    }
    
    if(key == 'a'){
        fakeRecorder->setAudioDeviceID( (fakeRecorder->getAudioDeviceID()+1) % audioDevices.size() );
    }
    if(key == 's'){
        bRevealSettings = true;
    }
    if(key == 'd'){
        bRevealSettings = false;
    }
}


//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){}

//--------------------------------------------------------------
void testApp::mouseReleased(){}

void testApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
    
}

/*****************************************************************************
 *	TOUCH EVENTS
 *****************************************************************************/

void testApp::TouchDown(Blob b){}

void testApp::TouchUp(Blob b){}

void testApp::TouchMoved(Blob b){}
