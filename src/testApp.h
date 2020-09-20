#ifndef _TEST_APP
#define _TEST_APP

//if standalone mode/non-addon
#define STANDALONE

//main
#include "ofMain.h"
//addon
#include "ofxKCore.h"



class testApp : public ofBaseApp, public TouchListener{
public:
	testApp(int argc, char *argv[])
	{
		debug = false;
		TouchEvents.addListener(this);
		if(argc==2)
		{
			printf("Command Line Option Passed : %s\n",argv[1]);
			if(strcmp(argv[1],"-d")==0)
			{
				debug = true;
			}
		}
	}
	ofxKCoreVision * ckv;
	bool debug;
    
	void setup();
	void update();
	void draw();
    void exit();
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased();
    
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
	
    //Touch Events
	void TouchDown(Blob b);
	void TouchMoved(Blob b);
	void TouchUp(Blob b);
    
    
    //Webcam
    ofVideoGrabber 		vidGrabber;
    
    //Recorder
    ofPtr<ofQTKitGrabber>	fakeRecorder; //THIS IS STILL NEEDED FOR VID RECORDER TO WORK
    vector<string> videoDevices;
    vector<string> audioDevices;
    
    
    void videoSaved(ofVideoSavedEventArgs& e);
    
    //Video
    ofVideoPlayer 		MemoryPlayback;
    
    
    //Blending
    ofBlendMode blendMode; //TAKE OUT IF NOT NEEDED
    
    
    //New recorder code
    
    
    bool bRecording;
    
    //string fileName;MAY WANT TO USE WHEN ITS MORE COMPLICATED
    //string fileExt;
    
    ofFbo recordFbo;
    ofPixels recordPixels;
    
    
    int iVidPlayCount = 0;
    int iTotalVideos = 3;
    bool bUpperHalf = false; //This is just a way to check how many videos are played
    
    //Video recording variables
    int iInteractionTimer = 0; //How long the person interacts for
    int iStartTimer = 0; //Moment video started recording
    int iVidTimer = 0; //Time of current video length
    int iTotalVidTime = 0; //Longest video length recoded so far
    
    bool bIncludeVid = false; //Checks to see if this video should be included
    bool bVidRecording = false; //States whether video is recording
    bool b0Record = true; //The way to check which video is being recorded over
    
    //Video saving and interacting methods
    void interaction();
    void saveVideo();
};



#endif

