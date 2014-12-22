#include "gnome.h"
#include "animator.h"
#include "toyfactory.h"
#include "santa.h"
#include "Defs.h"
#include <Windows.h>
#include <process.h>

#define GNOME_STATE_MOVE_LEFT 0
#define GNOME_STATE_MOVE_RIGHT 1
#define GNOME_STATE_MOVE_LEFT_PACKET 2
#define GNOME_STATE_MOVE_RIGHT_PACKET 3
#define ANIMATION_LENGTH 4
#define NUM_ANIMATIONS 4
#define MAX_PACKETS 4
#define PACKET_OFFSET 42

static const RECT _Clips[NUM_ANIMATIONS][ANIMATION_LENGTH] ={
        // GNOME_STATE_MOVE_LEFT
        { { 0, 128, 32, 160 }, { 32, 128, 64, 160 }, { 64, 128, 96, 160 }, { 96, 128, 128, 160 } },
        // GNOME_STATE_MOVE_RIGHT
        { { 128, 128, 160, 160 }, { 160, 128, 192, 160 }, { 192, 128, 224, 160 }, { 224, 128, 256, 160 } },
        // GNOME_STATE_MOVE_LEFT_PACKET
        { { 0, 160, 32, 192 }, { 32, 160, 64, 192 }, { 64, 160, 96, 192 }, { 96, 160, 128, 192 } },
        // GNOME_STATE_MOVE_RIGHT_PACKET
        { { 128, 160, 160, 192 }, { 160, 160, 192, 192 }, { 192, 160, 224, 192 }, { 224, 160, 256, 192 } },
};

static const RECT _PacketClips[MAX_PACKETS] ={
        { 128, 192, 160, 224 },
        { 160, 192, 192, 224 },
        { 192, 192, 224, 224 },
        { 224, 192, 256, 224 },
};

static animation_t* CreateGnomeAnimations( int *num ) {
    *num = NUM_ANIMATIONS;
    animation_t *animations = DBG_MALLOC( sizeof( animation_t ) * NUM_ANIMATIONS );

    for ( int i = 0; i < NUM_ANIMATIONS; ++i ) {
        animations[i].frameLength = 0.1f;
        animations[i].id = i;
        animations[i].loop = TRUE;
        animations[i].numClips = ANIMATION_LENGTH;
        animations[i].clips = malloc( sizeof( RECT ) * ANIMATION_LENGTH );
        memcpy( animations[i].clips, _Clips[i], sizeof( _Clips[i] ) );
    }

    return animations;
}

static unsigned int _stdcall ThreadFunc( void* );

typedef struct gnomeData_s {
    POINT *path;
    int pathLen;
    int step;
    animatedSprite_t sprite;
    sprite_t packet;
    ghandle_t packetHandle;
    toy_t toy;
} gnomeData_t;

// Sends a gnome to santa with a toy.
void GnomeStart( HBITMAP sprite, const POINT *path, int steps, toy_t toy ) {
    gnomeData_t *data = DBG_MALLOC( sizeof( gnomeData_t ) );
    data->toy = toy;
    data->path = DBG_MALLOC( sizeof( POINT ) * steps );
    memcpy( data->path, path, sizeof( POINT ) * steps );
    data->pathLen = steps;
    data->step = 0;

    data->sprite.animations = CreateGnomeAnimations( &data->sprite.numAnimations );
    data->sprite.sprite.bitmap = sprite;
    data->sprite.sprite.x = (float)path[0].x;
    data->sprite.sprite.y = (float)path[0].y;
    data->sprite.sprite.visible = TRUE;
    data->sprite.sprite.scaleX = 2.0f;
    data->sprite.sprite.scaleY = 2.0f;
    data->sprite.sprite.colorKey = RGB( 255, 0, 255 );

    data->sprite.nextPoint = 0;
    data->sprite.movementSequence = data->path;
    data->sprite.movementSequenceLength = steps;
    data->sprite.moveSpeed = 180.0f;

    data->sprite.spriteHandle = GraphicsRegisterSprite( &data->sprite.sprite );
    AnimatorSetAnimation( &data->sprite, GNOME_STATE_MOVE_LEFT_PACKET );
    GraphicsUpdateSprite( data->sprite.spriteHandle, &data->sprite.sprite );

    data->packet.bitmap = sprite;
    data->packet.clip = _PacketClips[rand() % MAX_PACKETS];
    data->packet.colorKey = RGB( 255, 0, 255 );
    data->packet.scaleX = 2.0f;
    data->packet.scaleY = 2.0f;
    data->packet.visible = TRUE;
    data->packet.x = data->sprite.sprite.x;
    data->packet.y = data->sprite.sprite.y - PACKET_OFFSET;

    data->packetHandle = GraphicsRegisterSprite( &data->packet );

    _beginthreadex(
        NULL,
        0,
        ThreadFunc,
        data,
        0,
        NULL );
}


static unsigned int _stdcall ThreadFunc( void *ptr ) {
    gnomeData_t *gnome = (gnomeData_t*)ptr;

    LARGE_INTEGER tick, lastTick, frequency;
    QueryPerformanceCounter( &lastTick );
    QueryPerformanceFrequency( &frequency );

    BOOL hasDelivered = FALSE;

    BOOL done = FALSE;
    while ( !done ) {

        QueryPerformanceCounter( &tick );
        const float DeltaTime = (float)( tick.QuadPart - lastTick.QuadPart ) / (float)frequency.QuadPart;

        AnimatorUpdate( &gnome->sprite, DeltaTime );

        if ( !hasDelivered ) {
            gnome->packet.x = gnome->sprite.sprite.x;
            gnome->packet.y = gnome->sprite.sprite.y - PACKET_OFFSET;
            GraphicsUpdateSprite( gnome->packetHandle, &gnome->packet );
        }
        // We've reached our destination.
        if ( gnome->sprite.nextPoint >= gnome->pathLen ) {
            OutputDebugString( "Gnome reached destination!\n" );

            if ( !hasDelivered ) {
                POINT *p0 = &gnome->path[0];
                POINT *p1 = &gnome->path[gnome->pathLen - 1];
                while ( p0 < p1 ) {
                    POINT tmp = *p0;
                    *p0 = *p1;
                    *p1 = tmp;
                    ++p0;
                    --p1;
                }
                gnome->sprite.nextPoint = 0;
                hasDelivered = TRUE;
                AnimatorSetAnimation( &gnome->sprite, GNOME_STATE_MOVE_RIGHT );
                
                SantaPutToy( gnome->toy );

                GraphicsUnRegisterSprite( gnome->packetHandle );
            } else {
                done = TRUE;
            }
        }

        lastTick = tick;
        Sleep( 5 );
    }

    GraphicsUnRegisterSprite( gnome->sprite.spriteHandle );

    for ( int i = 0; i < gnome->sprite.numAnimations; ++i ) {
        free( gnome->sprite.animations[i].clips );
    }
    free( gnome->sprite.animations );
    free( gnome->path );
    free( gnome );

    return 0;
}