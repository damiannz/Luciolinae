/*
 *  BufferedSerial.cpp
 *  serialExample
 *
 *  Created by damian on 07/05/10.
 *  Copyright 2010 frey damian@frey.co.nz. All rights reserved.
 *
 */

#include "BufferedSerial.h"

#include <assert.h>

static const int CHUNKSIZE=4;
static const int BLOCKSIZE=128; // at 9600 baud this is 8 blocks/second; one ack sent per block
static unsigned char ACK=0x7f;

// #define WAIT_FOR_ACK

void BufferedSerial::setup( ofSerial* _serial , int baud, float _delay )
{
	baudrate = baud;
	baud_timer = 0;
	bytes_written = 0;
	serial = _serial;
	sent_this_block = 0;
	
	delay_ms = _delay*1000;
	read_packet = 0;
	write_packet = 0;
	ready_packets = 0;
	
	
	ofLog( OF_LOG_NOTICE, "BufferedSerial:: setup() synchronising board/pc" );
	// clear out serial read buffer
	while ( serial->available() )
	{
		char rubbish = serial->readByte();
	}

	// synchronize buffer boundaries
	char dummy = 0;
	bool heard = false;
	int i = 0;
	
#ifdef WAIT_FOR_ACK
	// write BLOCKSIZE-1 bytes, wait for ack; repeat until you don't get an ack, then all is good
	while ( true )
	{
		// write BLOCKSIZE-1 bytes
		unsigned char dummy[BLOCKSIZE-1];
		memset( dummy, 0, BLOCKSIZE-1 );
		int to_write = BLOCKSIZE-1;
		while (to_write > 0 )
		{
			// try to write dummy
			int written = serial->writeBytes( dummy, to_write );
			if ( written <= 0 )
				delayUs( 100 );
			else
				to_write -= written;
		}
		
		// listen for an ack, wait 1 second
		int waited_count = 0;
		do {
			delayUs( 100*1000 );
		} while (!serial->available() && waited_count++ < 10 );
		// not got one?
		if ( !serial->available() )
		{
			// ok! good to go!
			sent_this_block = BLOCKSIZE-1;
			// write one more byte to trigger the ack
			writeBytes( dummy, 1 );
			break;
		}
		else
		{
			// get rid of the ack
			while ( serial->available() )
			{
				int acky = serial->readByte();
				if ( acky == ACK )
				{
					break;
				}
				delayUs( 100 );
			}
			// + try again
		}		
	}
#endif
	ofLog( OF_LOG_NOTICE, "BufferedSerial:: setup() finished" );
}

void BufferedSerial::shutdown()
{
#ifdef WAIT_FOR_ACK
	// write to the end of this block to synchronize buffer boundaries for next run
	unsigned char dummy[BLOCKSIZE-sent_this_block];
	memset( dummy, 0, BLOCKSIZE-sent_this_block );
	writeBytes( dummy, BLOCKSIZE-sent_this_block );
#endif
}

void BufferedSerial::beginWrite()
{
#ifdef WAIT_FOR_ACK
	assert( sent_this_block%CHUNKSIZE == 0);
	unsigned char dummy[2];
	dummy[0] = 0;
	dummy[1] = 0;
	writeBytes( dummy, 2 );
#endif
	
}

bool BufferedSerial::writeBytes( unsigned char* bytes, int size )
{
	if ( ready_packets == NUM_PACKETS )
		pushOutPacket();
	memcpy( packets[write_packet].data, bytes, size );
	packets[write_packet].count = size;
	packets[write_packet].timer = ofGetElapsedTimeMillis()+delay_ms;
	
	ready_packets++;
	write_packet++;
	if ( write_packet >= NUM_PACKETS )
		write_packet = 0;
	
	return true;
}

void BufferedSerial::pushOutPacket()
{
	if ( ready_packets == 0 )
		return;
	
    /*
    char buffer[4096];
    buffer[0] = 0;
    for ( int i=0; i<packets[read_packet].count; i++ )
        sprintf( buffer, "%s%02x ", buffer, packets[read_packet].data[i] );
    ofLog(OF_LOG_VERBOSE, "sending %s", buffer );*/
    
	writeBytes_real( packets[read_packet].data, packets[read_packet].count );
    
	ready_packets--;
	read_packet++;
	if ( read_packet >= NUM_PACKETS )
		read_packet = 0;
}

bool BufferedSerial::writeBytes_real( unsigned char* bytes, int size )
{
	int count = size;
	while ( count > 0 )
	{
		// write in BLOCKSIZE byte chunks
#ifdef WAIT_FOR_ACK
		int to_write = min(count,BLOCKSIZE-sent_this_block);
#else
		int to_write = count;
#endif
		int written = 0;
		while( to_write > 0 )
		{
			// try to send
			//ofLog( OF_LOG_VERBOSE, "BufferedSerial::writeBytes trying to send %i/%i bytes", to_write, count );
			written = serial->writeBytes( bytes, to_write );
			//ofLog( OF_LOG_VERBOSE, "BufferedSerial::writeBytes wrote %i bytes", written );
			if ( written <= 0 )
			{
				// sleep for 1ms and try again
				delayUs( 1000 );
			}
			else
			{
				// advance array
				bytes += written;
				to_write -= written;
				count -= written;
				sent_this_block += written;
				// wait for ack if necessary
				if ( sent_this_block >= BLOCKSIZE )
				{
#ifdef WAIT_FOR_ACK
					// wait for ack
					ofLog( OF_LOG_VERBOSE,"BufferedSerial::incBlockSentCount: waiting for ack" );
					// wait for serial availability
					while ( !serial->available() )
						delayUs( 100 );
					int ack;
					int read=0;
					while ( (ack=serial->readByte()) == OF_SERIAL_NO_DATA ) 
					{
						delayUs( 100 );
					} 
					if ( ack == OF_SERIAL_ERROR )
					{
						ofLog( OF_LOG_ERROR, "BufferedSerial::incBlockSentCount: non-recoverable error");
						return false;
					}
					if ( ack != 0x7f )
					{
						printf("BufferedSerial::incBlockSentCount: bad ack: %2x\n", ack );
						return false;
					}
#endif
					sent_this_block -= BLOCKSIZE;
				}
			}			
		}
	}
	
	bytes_written += size;
	return true;
}

void BufferedSerial::endWrite()
{
#ifdef WAIT_FOR_ACK
	if ( sent_this_block%CHUNKSIZE != 0 )
	{
		int sent_this_chunk = sent_this_block%CHUNKSIZE;
		// write dummy
		int to_send = CHUNKSIZE-sent_this_chunk;
		unsigned char dummy[to_send];
		memset( dummy, 0, to_send );
		//printf("endWrite writing extra %i bytes\n", to_send );
		writeBytes( dummy, to_send );
		// did it work?
		assert( sent_this_block % CHUNKSIZE == 0 );
	}
#else
	sent_this_block = 0;
#endif
	
	//serial->flush( /*in*/false, /*out*/true );
	// wait until the block has been sent
	if ( bytes_written > 32 )
	{
		serial->drain();
		bytes_written = 0;
	}
}


void BufferedSerial::update( float elapsed )
{
	baud_timer += elapsed;
	
	while ( ready_packets> 0 && packets[read_packet].timer < ofGetElapsedTimeMillis() )
	{
		pushOutPacket();
	}
	
	if ( bytes_written > 1024 )
	{
		// maintain baudrate
		float time_should_have_taken = (float(bytes_written)*10)/float(baudrate);
		time_should_have_taken *= 1.1f;
		float time_actually_took = baud_timer;
		// sleep if we have > 20ms lag
		if ( time_should_have_taken-time_actually_took > 0.02f )
		{
			float sleep_time = time_should_have_taken - time_actually_took;
			int sleep_time_us = sleep_time*1000.0f*1000.0f;
			//printf("BufferedSerial::update sleeping %i ms\n", sleep_time_us/1000 );
			delayUs( sleep_time_us );
			// clear a bit
			baud_timer -= (float(128)*10)/float(baudrate);
			bytes_written -= 128;
		}
	}
}	
