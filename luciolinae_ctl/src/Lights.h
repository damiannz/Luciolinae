/*
 *  Lights.h
 *  serialExample
 *
 *  Created by damian on 07/05/10.
 *  Copyright 2010 frey damian@frey.co.nz. All rights reserved.
 *
 */

#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"
#include "BufferedSerial.h"
#include <set>
#include <deque>
#include "Light.h"
class LightsDelaunay;


class Lights
{
public:
	Lights() { rebuild_delaunay = true; rebuild_boards_delaunay = true, delaunay = NULL; 	
		blend_state_stack.resize(1);
		disableBlending(); setBlendMode( BLEND_MIX ); setBlendAlpha( 1 );
		small_bright_factor = 1.0f; }
	~Lights();
	
	void setup( BufferedSerial* serial );
	void setup( BufferedSerial* serial, ofxXmlSettings &data );
	void save( ofxXmlSettings& data );

	void update( float elapsed );

	void draw();

	// all brightness settings are 0..1
	void pulse( int id, float max_bright, bool include_big = false, float end_bright=0 );
	void pulseAll( float max_bright, bool include_big = false );
	void set( int id, float bright, bool include_big = false );

	// all off; if pummel is true then do this very thoroughly
	void clear( bool pummel=false );
	
	// illuminate in this area
	void illuminateCircularArea( float x, float y, float area, bool include_big=false );
	// illuminate a line of area. (x,y) is a point on the line, (dx,dy) is a direction vector
	// and width is the illumination width of the line
	void illuminateCorridor( float x, float y, float dx, float dy, float power, float width, bool include_big=false );
	// only draw
	void drawIlluminateCorridor( float x, float y, float dx, float dy, float power, float width );
	// illuminate a rectangular area
	void illuminateRect( float x, float y, float w, float h, float power, bool include_big=false );
	
	// send all brightness commands immediately and latch in
	void flush();
	
	// accessors
	int getNumLights() const { return lights.size(); }
	const Light& getLight( int i ) const { return lights.at(i); }
	void setLightPosition( int id, float x, float y ) { lights.at(id).setPosition( x, y ); rebuild_boards_delaunay = true; rebuild_delaunay = true; }
	vector<int> getLightIdsForBoard( int board_id ) const;
	int getBoardIndexForId( int board_id ) const { return (board_id/16)-1; }
	int getBoardIdForIndex( int board_index ) const { return (board_index+1)*16; }
	
	// big lights (high current)
	int getNumBigLights() const { return big_lights.size(); }
	// we have 2 sets of indices: big light indices [0..num big lights], and light indices [0..num lights]
	// get the ith big light
	const Light& getBigLight( int i ) const { return lights[big_lights.at(i)]; }
	// get the light index for the ith big light index
	int getBigLightIndex( int i ) const { return big_lights.at(i); }
	// get the big light index for the given light index, or -1 if this isn't a big light
	int getLightIndexForBig( int i ) const;
	void toggleLightIsBig( int id );
	
	// groups
	void toggleGroupMembership( int id, int group_id ) {};

	// small lights
	void increaseSmallLightBrightnessFactor() { small_bright_factor *= 1.1f; }
	void decreaseSmallLightBrightnessFactor() { small_bright_factor /= 1.1f; }
	float getSmalLightBrightnessFactor() { return small_bright_factor; }
	
	// get delaunay triangulation
	LightsDelaunay* getDelaunay();
	
	// get delaunay triangulation for just one board
	LightsDelaunay* getBoardDelaunay( int board_id );
	
	// blending
	void enableBlending() { blend_state_stack.back().blending = true; }
	void disableBlending() { blend_state_stack.back().blending = false; }
	// blend mode
	typedef enum _BlendMode{ BLEND_ADD, BLEND_MIX } BlendMode;
	void setBlendMode( BlendMode _mode ) { blend_state_stack.back().blend_mode = _mode; }
	void setBlendAlpha( float a ) { blend_state_stack.back().blend_alpha = a; }
	// blending state stack
	void pushBlendState() { blend_state_stack.push_back( blend_state_stack.back() ); }
	void popBlendState() { blend_state_stack.pop_back(); }

private:
	// takes brightness of 0..4095
	void sendLightLevel( unsigned char board_id, unsigned char light_id, int brightness );
	// takes brightness of 0..4095
	// decay is in steps (1/4096'th) per millisecond
	void sendPulse( unsigned char board_id, unsigned char light_id, int brightness, unsigned char decay );
	// send packed data
	void sendEveryLightLevel( unsigned char board_id, unsigned char data[24] );
	// latch in
	void latch( unsigned char board_id=0 /* default all */ );

	// culcalet crc of given data
	unsigned char calculateCRC( unsigned char* data, int length ) { return updateCRC( data, length, 0 ); };
	// update current_crc by crc-ing the given data, assumed to be appended
	unsigned char updateCRC( unsigned char* data, int length, unsigned char current_crc );
	
	BufferedSerial* serial;
	LightsDelaunay* delaunay;
	map<int, LightsDelaunay*> boards_delaunay;
	
	int num_boards;
	bool rebuild_delaunay;
	bool rebuild_boards_delaunay;
	
	
	vector<Light> lights;

	vector<int> big_lights;
	
	
	typedef struct _BlendState
	{
		bool blending;
		BlendMode blend_mode;
		float blend_alpha;
	} BlendState;
	deque<BlendState> blend_state_stack;
	
	float small_bright_factor;
	
	vector<int> debug_sent;
	

	
};

