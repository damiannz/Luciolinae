#include "testApp.h"
#include "AnimSweep.h"
#include "AnimPositionCalibrate.h"
#include "AnimDelaunay.h"
#include "AnimStateMachine.h"
#include "AnimSeq.h"
#include "AnimSeqSine.h"
#include "StateAnimIdle.h"
#include "AnimKapelica.h"
#include "AnimGazebo.h"

#define DO_PD

//--------------------------------------------------------------
void testApp::setup(){	 

	printf("testApp::setup()\n");

	shutdown_started = false;
	
#ifndef NO_WINDOW
	ofSetVerticalSync(true);
	ofBackground(20,20,20);	
	ofEnableAlphaBlending();
	ofSetFrameRate( 60 );
#endif
	
#ifdef NO_WINDOW
	ofSetDataPathRoot( "data/" );
	ofSetFrameRate( 60 ); 
#endif
	
	ofSetLogLevel( OF_LOG_NOTICE );
	
	serial.enumerateDevices();
#ifdef TARGET_LINUX
	static const char* SERIAL_PORT = "/dev/ttyUSB0";
#elif defined TARGET_OSX
	static const char* SERIAL_PORT = "/dev/tty.usbserial-FTT8R2AA";
	//static const char* SERIAL_PORT = "/dev/tty.usbserial-0000103D";
	//static const char* SERIAL_PORT = "/dev/tty.usbserial-A4000R0L";
	//static const char* SERIAL_PORT	 = "/dev/tty.usbserial-FTTEIQQY";
#endif
	static const int BAUDRATE = 19200;
	if ( serial.setup(SERIAL_PORT, BAUDRATE ) )
		buffered_serial = new BufferedSerial();
	else
	{
		printf("couldn't open serial %s, making dummy\n", SERIAL_PORT );
		buffered_serial = new BufferedSerialDummy();
	}
	buffered_serial->setup( &serial, BAUDRATE );

	// osc
	static const char* OSC_HOST = "localhost";
	static const int OSC_PORT = 10001;
	osc.setup( OSC_HOST, OSC_PORT );
	
	
	// load settings
	ofxXmlSettings data;
	if ( data.loadFile( "settings.xml" ) )
		lights.setup( buffered_serial, data );
	else
	{
		ofLog( OF_LOG_ERROR, "couldn't load data/settings.xml");
		lights.setup( buffered_serial );
	}
	
	ontime_ms = data.getValue("ontime_ms", 0 );
	printf("got ontime_ms: %i\n", ontime_ms );
	
	
	data.popTag();
	

	AnimationFactory::useLights( &lights );
    
    anim_switcher.addAnim( AnimSeq::NAME );

	anim_switcher.addAnim( AnimGazebo::NAME );
	anim_switcher.addAnim( AnimKapelica::NAME );
	anim_switcher.addAnim( AnimStateMachine::NAME );
	anim_switcher.addAnim( AnimSweep::NAME );
	anim_switcher.addAnim( AnimDelaunay::NAME );
	anim_switcher.addAnim( AnimPositionCalibrate::NAME );
	anim_switcher.addAnim( AnimSeq::NAME );
	anim_switcher.addAnim( AnimSeqSine::NAME );

	current_anim = anim_switcher.goToAnim( AnimSeq::NAME );
	
#ifdef DO_PD
	printf("starting pd...\n");
	
	pd.setup( "" );
	//pd.addOpenFile( "pd-test.pd" );
	pd.addOpenFile( "pdstuff/_main.pd" );
	pd.start();

	ofSoundStreamSetup(2, 0, this, 44100, 256, 4 );
#endif
	
	ofSoundStreamSetup(2, 0, this, 44100, 256, 12 );
	printf("testApp::setup() finished\n");
	
}

//--------------------------------------------------------------
void testApp::update(){
	buffered_serial->update( ofGetLastFrameTime() );
	breath.update( ofGetLastFrameTime() );
	
	// update delaunay pulses
	pulses.update( ofGetLastFrameTime() );
	

	// update lights
	lights.update( ofGetLastFrameTime() );
	
	
	// update animation
	current_anim->update( ofGetLastFrameTime() );

	//lights.illuminateCircularArea( float(mouseX)/ofGetWidth(), float(mouseY)/ofGetHeight(), illArea );

	// send light levels
	lights.flush();
	
	
	static const int FADE_TIME = 10000;
	static const int STARTUP_DELAY = 5000;
	// fade up
	if ( ofGetElapsedTimeMillis()-STARTUP_DELAY < FADE_TIME )
	{
		// calculate a volume
		float vol = float(ofGetElapsedTimeMillis()-STARTUP_DELAY)/FADE_TIME;
		vol = max(0.0f, vol);
		// send volume
		ofxOscMessage m;
		m.setAddress("/volume");
		m.addFloatArg( vol );
		osc.sendMessage(m);
	}
	else if ( ontime_ms > 0 && ofGetElapsedTimeMillis() > ontime_ms )
{
		// calculate a volume
		float vol = float(ofGetElapsedTimeMillis()-ontime_ms)/FADE_TIME;
		vol = min(1.0f,max(0.0f, vol));
		vol = 1.0f-vol;
		// send volume
		ofxOscMessage m;
		m.setAddress("/volume");
		m.addFloatArg( vol );
		osc.sendMessage(m);
		
		if ( ontime_ms >0 && (ofGetElapsedTimeMillis() > (ontime_ms+FADE_TIME)) )
		{
			printf("quit time\n");
			::exit(0);
		}
	}

	
	
}

//--------------------------------------------------------------
void testApp::draw(){
#ifndef NO_WINDOW
	lights.draw();
	current_anim->draw();
	anim_switcher.draw();
#endif
}

void testApp::exit()
{
	pd.stop();
	printf("clearing lights\n");
	lights.clear( true );
	buffered_serial->shutdown();
}

//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	
	switch( key )
	{
		// panic button
		case '!':
			lights.clear( true );
			break;
		case 'p':
			lights.pulseAll( 1 );
			break;
		case 'W':
		{
			ofxXmlSettings data;
			lights.save( data );
			data.saveFile( "settings.xml" );
			data.addValue("ontime_ms", ontime_ms);
			break;
		}
		case OF_KEY_LEFT:
		case '[':
			current_anim = anim_switcher.prevAnim();
			lights.clear();
			break;
		case OF_KEY_RIGHT:
		case ']':
			current_anim = anim_switcher.nextAnim();
			lights.clear();
			break;
		
		case 'b':
			lights.increaseSmallLightBrightnessFactor();
			break;
		case 'B':
			lights.decreaseSmallLightBrightnessFactor();
			break;
		
		case 'R':
			ofSeedRandom( 12345 );
			break;
		case 'r':
			StateAnimIdle::should_reset_random = true;
			
			break;
			
		default:
			break;
			
	}
	current_anim->keyPressed( key );
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){ 
	
}

//-------------------------------------------------------------- 
void testApp::mouseMoved(int x, int y ){
	current_anim->mouseMoved( x, y );
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}


void testApp::audioRequested( float* input, int bufferSize, int nChannels )
{
	pd.renderAudio( input, bufferSize, nChannels );
		
}
