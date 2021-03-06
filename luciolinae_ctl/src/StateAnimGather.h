/*
 *  StateAnimGather.h
 *  luciolinae_ctl
 *
 *  Created by damian on 20/05/10.
 *  Copyright 2010 frey damian@frey.co.nz. All rights reserved.
 *
 */

#pragma once

#include "StateAnimation.h"
#include "Util.h"

class StateAnimGather : public StateAnimation
{
public:
	StateAnimGather( Lights* _lights ) : StateAnimation( _lights ) {};
	
	static const char* NAME;
	string getName() { return NAME; }
	
	void update( float elapsed );
	void draw() {};
	
	void enter() { 
		// blip the appropriate light
		int which = SharedData::getFloat( "blip_target" );
		count = ofRandom( 2, 7 );
		timer = squaredRandom( 0.1f, 0.5f ); 
	}
	bool isFinished() { return timer < 0; }
	
private:
	
	int count;
	float timer;
	
};

