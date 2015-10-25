#include "ofApp.h"
#include "util/split.h"
#include <Poco/Mutex.h>
#include <Poco/TemporaryFile.h>
#include "globals.h"
#include <cctype> 
Poco::Mutex mutex;
Poco::Mutex updateMutex;

bool applicationRunning = false;

//--------------------------------------------------------------
void ofApp::setup(){
	dropped = 0;
	changed = false;
	clearFbos = false;
	lastMouseMoved = 0;
	ofSetVerticalSync(true);
	ofBackground(0);
	ofSetBackgroundAuto(false);
	shader.setGeometryInputType(GL_LINES);
	shader.setGeometryOutputType(GL_QUADS);
	shader.setGeometryOutputCount(4);
	shader.load("shaders/osci.vert", "shaders/osci.frag", "shaders/osci.geom");
	
	vector<RtAudio::DeviceInfo> devices = listRtSoundDevices();
	ofSetFrameRate(60);
	
	root = new mui::Root();
	
	globals.loadFromFile();
	globals.player.loadSound( "konichiwa.wav" );
	globals.player.setLoop(true);
	globals.player.stop(); 
	
	configView = new ConfigView();
	configView->fromGlobals();
	if( globals.autoDetect ){
		configView->autoDetect();
	}

	root->add( configView );
	
	osciView = new OsciView();
	osciView->visible = false;
	root->add( osciView );
	
	left.loop = false;
	right.loop = false;

	
	if( globals.autoDetect ){
		startApplication();
	}

	windowResized(ofGetWidth(), ofGetHeight());
}


void ofApp::startApplication(){
	if( applicationRunning ) return;
	applicationRunning = true;
	
	left.play();
	right.play();

	configView->toGlobals();
	globals.saveToFile();
	configView->visible = false;
	osciView->fromGlobals();
	osciView->visible = true;
	
	if( globals.autoDetect ){
		cout << "Running auto-detect for sound cards" << endl;
		getDefaultRtOutputParams( globals.deviceId, globals.sampleRate, globals.bufferSize, globals.numBuffers );
	}
	
	//if you want to set the device id to be different than the default
	cout << "Opening Sound Card: " << endl;
	cout << "    Sample rate: " << globals.sampleRate << endl;
	cout << "    Buffer size: " << globals.bufferSize << endl;
	cout << "    Num Buffers: " << globals.numBuffers << endl;
	
	soundStream.setDeviceID( globals.deviceId );
	soundStream.setup(this, 2, 0, globals.sampleRate, globals.bufferSize, globals.numBuffers);
	globals.player.setupAudioOut(2, globals.sampleRate);
}


void ofApp::stopApplication(){
	configView->fromGlobals();
	globals.flipXY = globals.flipXY;
	globals.invertX = globals.invertX;
	globals.invertY = globals.invertY;

	globals.scale = osciView->scaleSlider->slider->value;
	globals.saveToFile();
	
	if( !applicationRunning ) return;
	applicationRunning = false;
	soundStream.stop();
	soundStream = ofSoundStream();
	configView->visible = true;
	osciView->visible = false;
}



//--------------------------------------------------------------
void ofApp::update(){
	
	// nasty hack. OF seems to get width+height wrong on the first frame.
	if( ofGetFrameNum() == 1 ){
		windowResized(ofGetWidth(), ofGetHeight());
	}
	
/*	if( ofGetElapsedTimeMillis()-lastMouseMoved > 5000 && globals.player.isPlaying ){
		osciView->visible = false;
	}*/

	if( !applicationRunning ){
		ofShowCursor();
		return;
	}
	if( osciView->visible ) ofShowCursor();
	else ofHideCursor();
	
	changed = false;
	shapeMesh.clear();
	shapeMesh.setMode(OF_PRIMITIVE_LINES);
	shapeMesh.enableColors();
	
	const int bufferSize = 512*4;
	static float * leftBuffer = new float[bufferSize];
	static float * rightBuffer = new float[bufferSize];

	if( left.totalLength >= bufferSize && right.totalLength >= bufferSize ){
		changed = true;
		while( left.totalLength >= bufferSize && right.totalLength >= bufferSize ){
			memset(leftBuffer,0,bufferSize*sizeof(float));
			memset(rightBuffer,0,bufferSize*sizeof(float));
			
			left.addTo(leftBuffer, 1, bufferSize);
			right.addTo(rightBuffer, 1, bufferSize);
			left.peel(bufferSize);
			right.peel(bufferSize);
			
			ofColor col = ofColor::fromHsb(globals.hue*255/360, 255, 255*globals.intensity);
			
			if( shapeMesh.getVertices().size() < bufferSize*2 ){
				for( int i = 1; i < bufferSize; i++ ){
					ofPoint a = ofPoint(leftBuffer[i-1], rightBuffer[i-1]);
					ofPoint b = ofPoint(leftBuffer[i  ], rightBuffer[i  ]);
					shapeMesh.addVertex(a);
					shapeMesh.addVertex(b);
				}
			}
			else{
				dropped ++;
			}
		}
	}
}

ofMatrix3x3 ofApp::getViewMatrix() {
	ofMatrix3x3 viewMatrix = ofMatrix3x3(
		globals.scale, 0.0, 0.0,
		0.0, -globals.scale, 0.0,
		0.0, 0.0, 1.0);

	if (globals.invertX) viewMatrix[0] *= -1;
	if (globals.invertY) viewMatrix[4] *= -1;

	if (globals.flipXY) {
		viewMatrix = ofMatrix3x3(
			0.0, 1.0, 0.0,
			1.0, 0.0, 0.0,
			0.0, 0.0, 1.0) * viewMatrix;
	}

	ofMatrix3x3 aspectMatrix = ofMatrix3x3(
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0);

	{
		float aspectRatio = float(ofGetWidth()) / float(ofGetHeight());
		if (aspectRatio > 1.0) {
			aspectMatrix[0] /= aspectRatio;
		}
		else {
			aspectMatrix[4] *= aspectRatio;
		}
	}

	return viewMatrix * aspectMatrix;
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofClear(0,255);
	
	if( !fbo.isAllocated() || fbo.getWidth() != ofGetWidth() || fbo.getHeight() != ofGetHeight() ){
		fbo.allocate(ofGetWidth(), ofGetHeight(),GL_RGBA);
		fbo.begin();
		ofClear(0,255);
		fbo.end();
	}
	
	if( changed && globals.player.isPlaying ){
		fbo.begin();
		ofSetColor( 0, (1-globals.afterglow)*255 );
		ofFill();
		ofDrawRectangle( 0, 0, ofGetWidth(), ofGetHeight() );
	
		ofEnableAlphaBlending();
		ofMatrix3x3 viewMatrix = getViewMatrix();

//      TODO: draw the cross section
//		ofSetColor(255, 0, 0, 25);
//		ofDrawLine( -10, 0, 10, 0 );
//		ofDrawLine( 0, -10, 0, 10 );
		
		shapeMesh.enableColors();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		shader.begin();
		shader.setUniform1f("uSize", globals.strokeWeight / 1000.0);
		shader.setUniform1f("uIntensity", globals.intensity);
		shader.setUniformMatrix3f("uMatrix", viewMatrix);
		shader.setUniform1f("uHue", globals.hue );
		ofSetColor(255);
		shapeMesh.draw();
		shader.end();
		ofEnableAlphaBlending();

		fbo.end();
	}
	
	ofSetColor(150);
	fbo.draw(0,0);
	ofDrawBitmapString("Dropped: " + ofToString(dropped), 10, 20 );
}

void exit_from_c(){
	exit(0); 
}

void ofApp::exit(){
	stopApplication();
	exit_from_c();
}


//----------------------------------------------------------	----
void ofApp::keyPressed  (int key){
	lastMouseMoved = ofGetElapsedTimeMillis();
	key = std::tolower(key);
	
	if( key == '\t' && !configView->isVisibleOnScreen()){
		osciView->visible = !osciView->visible;
	}

	if( key == 'f' || key == OF_KEY_RETURN || key == OF_KEY_F11 ){
		// nasty!
		osciView->fullscreenButton->clickAndNotify(); 
	}
	
	if( key == OF_KEY_ESC ){
		osciView->fullscreenButton->clickAndNotify(false);
	}
	
	if( key == ' '  ){
		osciView->playButton->clickAndNotify();
	}
	
	if( key == 'r' ){
		clearFbos = true;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	lastMouseMoved = ofGetElapsedTimeMillis();
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	lastMouseMoved = ofGetElapsedTimeMillis();
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	lastMouseMoved = ofGetElapsedTimeMillis();
}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
	osciView->width = min(500,w/mui::MuiConfig::scaleFactor);
	osciView->layout();
	osciView->x = w/mui::MuiConfig::scaleFactor/2 - osciView->width/2;
	osciView->y = h/mui::MuiConfig::scaleFactor - osciView->height - 20;
}

//--------------------------------------------------------------
void ofApp::audioIn(float * input, int bufferSize, int nChannels){
	if( !globals.player.isLoaded ){
		left.append(input, bufferSize,2);
		right.append(input+1,bufferSize,2);
	}
}

void ofApp::audioOut( float * output, int bufferSize, int nChannels ){
	memset(output, 0, bufferSize*nChannels);
	if( globals.player.isLoaded ){
		globals.player.audioOut(output, bufferSize, nChannels);
		
		left.append(output, bufferSize,2);
		right.append(output+1,bufferSize,2);
		
		AudioAlgo::scale(output, globals.outputVolume, nChannels*bufferSize);
		
	}
	else{
		
	}
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
	if( msg.message == "start-pressed" ){
		startApplication();
	}
	else if( msg.message == "stop-pressed" ){
		stopApplication();
	}
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
	cout << "files: " << endl;
	for( vector<string>::iterator it = dragInfo.files.begin();it != dragInfo.files.end(); ++it ){
		cout << *it << endl;
	}
	
	if( dragInfo.files.size() >= 1 ){
		globals.player.loadSound(dragInfo.files[0]);
	}
	
}
