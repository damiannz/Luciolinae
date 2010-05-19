/*
 *  Animation.cpp
 *  serialExample
 *
 *  Created by damian on 07/05/10.
 *  Copyright 2010 frey damian@frey.co.nz. All rights reserved.
 *
 */

#include "Animation.h"
#include "AnimActivators.h"
#include "AnimPositionCalibrate.h"
#include "AnimFlicker.h"
#include "AnimLuxeonTest.h"
#include "AnimSweep.h"
#include "AnimDelaunay.h"
#include "AnimStateMachine.h"
#include "StateAnimIdle.h"
#include "StateAnimBlip.h"
#include "StateAnimStartBlip.h"


map< string, Animation* > AnimationFactory::cache;
Lights* AnimationFactory::lights = NULL;

Animation* AnimationFactory::makeAnimation( string name )
{
	assert( lights != NULL );
	if ( cache.find(name) == cache.end() )
	{
		if ( name == AnimActivators::NAME )
			cache[name] = new AnimActivators( lights );
		else if ( name == AnimPositionCalibrate::NAME )
			cache[name] = new AnimPositionCalibrate( lights );
		else if ( name == AnimFlicker::NAME )
			cache[name] = new AnimFlicker( lights );
		else if ( name == AnimSweep::NAME )
			cache[name] = new AnimSweep( lights );
		else if ( name == AnimDelaunay::NAME )
			cache[name] = new AnimDelaunay( lights );
		
		else if ( name == AnimStateMachine::NAME )
			cache[name] = new AnimStateMachine( lights );
		
		else if ( name == StateAnimIdle::NAME )
			cache[name] = new StateAnimIdle( lights );
		else if ( name == StateAnimStartBlip::NAME )
			cache[name] = new StateAnimStartBlip( lights );
		else if ( name == StateAnimBlip::NAME )
			cache[name] = new StateAnimBlip( lights );
		
		/*
		else if ( name == StateAnimIdle::NAME )
			cache[name] = new StateAnimIdle( lights );*/
		else
		{
			printf("AnimationFactory: unknown animation name '%s'\n", name.c_str() );
			assert( false );
			return new Animation( lights );
		}
	}
	return cache[name];
};
