#pragma once

#include "ofxMightyUI.h"
#include "../globals.h"
#include "FaButton.h"
#include "FaToggleButton.h"
#include "FMenu.h"


class PlayerOverlay : public mui::Container{
public:
	PlayerOverlay( float x_ = 0, float y_ = 0, float width_ = ofGetWidth()/mui::MuiConfig::scaleFactor, float height_ = ofGetHeight()/mui::MuiConfig::scaleFactor );
	
	virtual void fromGlobals(); 
	
	virtual void update();
	virtual void draw();
	virtual void layout(); 
	
	virtual void touchDown( ofTouchEventArgs &touch );
	virtual void touchMoved( ofTouchEventArgs &touch );
	virtual void touchUp( ofTouchEventArgs &touch );
	virtual void touchDoubleTap( ofTouchEventArgs &touch );
	
	FMenu * micMenu; 
	map<string,int> micDeviceIds;
	
	FaButton * loadFileButton; 
	FaToggleButton * useMicButton;
	
	FaButton * stopButton;
	mui::SliderWithLabel * scaleSlider;
	
	FaToggleButton * fullscreenToggle; // toggle fs
	FaToggleButton * flipXYToggle; // flip x with y axis
	FaToggleButton * invertXToggle; // mirror around x axis
	FaToggleButton * invertYToggle; // mirror around y axis
	FaToggleButton * sideBySideToggle; // for 3d: toggle side by side mode and anagylph3d mode
	FaToggleButton * flip3dToggle; // for 3d: swap left and right channels
	FaToggleButton * zModulationToggle; // enable variable brightness control in 3 channel files
	FaToggleButton * showPlaylistToggle; // show playlist

	FaToggleButton * playButton;
	mui::Slider * timeSlider;
	mui::Button * timeLabelButton;
	int timeLabelMode = 0; 
	
	mui::Label currentTime;
	mui::Label * scaleLabel;
	
	mui::Label * outputVolumeLabel;
	mui::SliderWithLabel * outputVolumeSlider;
	
	mui::SliderWithLabel * strokeWeightSlider;
	mui::Label * strokeWeightLabel;
	
	mui::SliderWithLabel * timeStretchSlider;
	mui::Label * timeStretchLabel;
	
	mui::SliderWithLabel * blurSlider;
	mui::Label * blurLabel;
	
	mui::SliderWithLabel * numPtsSlider;
	mui::Label * numPtsLabel;
	
	mui::SliderWithLabel * hueSlider;
	mui::Label * hueLabel;
	
	mui::SliderWithLabel * intensitySlider;
	mui::Label * intensityLabel;
	
	mui::SliderWithLabel * afterglowSlider;
	mui::Label * afterglowLabel;
	
	void buttonPressed( const void * sender, ofTouchEventArgs & args );
	void sliderChanged( const void * sender, float & value );
	
private:
	mui::Label * addLabel( string text );
};
