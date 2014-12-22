#include "santa.h"
#include "graphics.h"
#include "animator.h"
#include "monitor.h"
#include "toyfactory.h"
#include <Windows.h>
#include <process.h>


#define MAX_ANIM_LENGTH 4
#define SANTA_MOVE_ANIM_TIME 0.1f
#define SANTA_SLEEP_ANIM_TIME 0.8f
#define ANIM_MIRROR_OFFSET 4

#define SANTA_STATE_MOVE_LEFT 0
#define SANTA_STATE_MOVE_RIGHT 1
#define SANTA_STATE_MOVE_LEFT_BAG 2
#define SANTA_STATE_MOVE_RIGHT_BAG 3
#define SANTA_STATE_ASLEEP 4

#define SANTA_ROUTE_DELIVER 0
#define SANTA_ROUTE_BACK 1
#define SANTA_ROUTE_WAIT 2

#define MAX_SACK_WEIGHT 10.0f
#define MAX_SACK_VOLUME 5.0f
#define MAX_SACK_ITEMS 12

static unsigned int _stdcall ThreadFunc( void * );

int g_santaMonitor;

static uintptr_t _thread;
//static sprite_t _sprite;
static animatedSprite_t _sprite;
static int _santaState;
static int _currentClip;
static int _isRunning;
static ghandle_t _spriteHandle;
static int _currentRoute;
static int _previousRoute;
static float _waitTimer;
static toy_t _sack[MAX_SACK_ITEMS];
static int _sackIndex;
static sprite_t _sackSprite;
static ghandle_t _sackHandle;

static const RECT _Clips[][MAX_ANIM_LENGTH] ={
    // SANTA_STATE_MOVE_LEFT
        { { 0, 0, 32, 32 }, { 32, 0, 64, 32 }, { 64, 0, 96, 32 }, { 96, 0, 128, 32 } },
        // SANTA_STATE_MOVE_RIGHT
        { { 128, 0, 160, 32 }, { 160, 0, 192, 32 }, { 192, 0, 224, 32 }, { 224, 0, 256, 32 } },
        // SANTA_STATE_MOVE_LEFT_BAG
        { { 0, 32, 32, 64 }, { 32, 32, 64, 64 }, { 64, 32, 96, 64 }, { 96, 32, 128, 64 } },
        // SANTA_STATE_MOVE_RIGHT
        { { 128, 32, 160, 64 }, { 160, 32, 192, 64 }, { 192, 32, 224, 64 }, { 224, 32, 256, 64 } },
        // SANTA_STATE_ASLEEP
        { { 0, 64, 32, 96 }, { 32, 64, 64, 96 }, { 64, 64, 96, 96 }, { 96, 64, 128, 96 } },
};

static const RECT _SackClips[] ={
        { 0, 96, 32, 128 }, { 32, 96, 64, 128 }, { 64, 96, 96, 128 }, { 96, 96, 128, 128 }
};

static const POINT _SackPosition = { 392, 182 };

static const POINT _Routes[][2] ={
        // SANTA_ROUTE_DELIVER
        { { 48, 224 }, { 48, -64 } },
        // SANTA_ROUTE_BACK
        { { 48, 224 }, { 376, 224 } }
};


void SantaStart( HBITMAP sprite ) {

    _sprite.sprite.bitmap = sprite;
    _sprite.sprite.visible = TRUE;

    _santaState = SANTA_STATE_ASLEEP;
    _currentClip = 0;
    _isRunning = TRUE;

    animation_t *animations = malloc( sizeof( animation_t ) * 5 );

    // SANTA_STATE_MOVE_LEFT
    animations[0].frameLength = 0.1f;
    animations[0].id = SANTA_STATE_MOVE_LEFT;
    animations[0].loop = TRUE;
    animations[0].numClips = 4;
    animations[0].clips = malloc( sizeof( RECT ) * 4 );
    memcpy( animations[0].clips, _Clips[0], sizeof( _Clips[0] ) );

    // SANTA_STATE_MOVE_RIGHT
    animations[1].frameLength = 0.1f;
    animations[1].id = SANTA_STATE_MOVE_RIGHT;
    animations[1].loop = TRUE;
    animations[1].numClips = 4;
    animations[1].clips = malloc( sizeof( RECT ) * 4 );
    memcpy( animations[1].clips, _Clips[1], sizeof( _Clips[1] ) );

    // SANTA_STATE_MOVE_LEFT_BAG
    animations[2].frameLength = 0.1f;
    animations[2].id = SANTA_STATE_MOVE_LEFT_BAG;
    animations[2].loop = TRUE;
    animations[2].numClips = 4;
    animations[2].clips = malloc( sizeof( RECT ) * 4 );
    memcpy( animations[2].clips, _Clips[2], sizeof( _Clips[2] ) );

    // SANTA_STATE_MOVE_RIGHT_BAG
    animations[3].frameLength = 0.1f;
    animations[3].id = SANTA_STATE_MOVE_RIGHT_BAG;
    animations[3].loop = TRUE;
    animations[3].numClips = 4;
    animations[3].clips = malloc( sizeof( RECT ) * 4 );
    memcpy( animations[3].clips, _Clips[3], sizeof( _Clips[3] ) );

    // SANTA_STATE_ASLEEP
    animations[4].frameLength = 0.4f;
    animations[4].id = SANTA_STATE_ASLEEP;
    animations[4].loop = TRUE;
    animations[4].numClips = 4;
    animations[4].clips = malloc( sizeof( RECT ) * 4 );
    memcpy( animations[4].clips, _Clips[4], sizeof( _Clips[4] ) );

    _sprite.animations = animations;
    _sprite.numAnimations = 5;

    AnimatorSetAnimation( &_sprite, SANTA_STATE_ASLEEP );

    // Colorkey is magenta
    _sprite.sprite.colorKey = RGB( 255, 0, 255 );

    _sprite.sprite.scaleX = 2.0f;
    _sprite.sprite.scaleY = 2.0f;
    _sprite.sprite.x = 380.0f;
    _sprite.sprite.y = 224.0f;

    _sprite.spriteHandle = GraphicsRegisterSprite( &_sprite.sprite );

    _sprite.nextPoint = 1;
    _sprite.movementSequence = _Routes[1];
    _sprite.movementSequenceLength = 2;
    _sprite.moveSpeed = 150.0f;

    _currentRoute = SANTA_ROUTE_WAIT;
    _previousRoute = SANTA_ROUTE_BACK;
    _waitTimer = 0.0f;

    _sackSprite.bitmap = sprite;
    _sackSprite.colorKey = RGB( 255, 0, 255 );
    _sackSprite.visible = TRUE;
    _sackSprite.x = (float)_SackPosition.x;
    _sackSprite.y = (float)_SackPosition.y;
    _sackSprite.clip = _SackClips[0];
    _sackSprite.scaleX = 2.0f;
    _sackSprite.scaleY = 2.0f;

    _sackHandle = GraphicsRegisterSprite( &_sackSprite );
    GraphicsUpdateSprite( _sackHandle, &_sackSprite );

    _thread = _beginthreadex(
        NULL,
        0,
        ThreadFunc,
        NULL,
        0,
        NULL );
}

void SantaStop() {
    _isRunning = FALSE;
    
    WaitForSingleObject( (HANDLE)_thread, INFINITE );

    for ( int i = 0; i < _sprite.numAnimations; ++i ) {
        free( _sprite.animations[i].clips );
    }
    free( _sprite.animations );
}

static void GetSackMetrics( float *weight, float *volume ) {

    float totalWeight = 0.0f;
    float totalVolume = 0.0f;

    for ( int i = 0; i < _sackIndex; ++i ) {
        totalWeight += _sack[i].weight;
        totalVolume += _sack[i].volume;
    }

    *weight = totalWeight;
    *volume = totalVolume;
}

void SantaPutToy( toy_t toy ) {
    MonitorEnter( &g_santaMonitor );

    float weight, volume;
    GetSackMetrics( &weight, &volume );

    while ( weight > MAX_SACK_WEIGHT || volume > MAX_SACK_VOLUME || _sackIndex >= MAX_SACK_ITEMS ||
        _currentRoute == SANTA_ROUTE_DELIVER ) {
        MonitorWait( &g_santaMonitor );
        GetSackMetrics( &weight, &volume );
    }

    _sack[_sackIndex++] = toy;

    MonitorLeave( &g_santaMonitor );
}

static unsigned int _stdcall ThreadFunc( void *ptr ) {
    // TODO(Peter): Threading stuff.

    // Initialize Timing
    LARGE_INTEGER tick, lastTick, frequency;
    QueryPerformanceFrequency( &frequency );
    QueryPerformanceCounter( &lastTick );

    float animTimer = 0.0f;

    while ( _isRunning ) {

        QueryPerformanceCounter( &tick );

        const float DeltaTime = (float)( tick.QuadPart - lastTick.QuadPart ) / (float)( frequency.QuadPart );

        AnimatorUpdate( &_sprite, DeltaTime );

        _waitTimer += DeltaTime;

        if ( _sprite.nextPoint > 1 ) {

            if ( _currentRoute == SANTA_ROUTE_WAIT ) {
                if ( _previousRoute == SANTA_ROUTE_DELIVER ) {
                    _currentRoute = SANTA_ROUTE_BACK;
                    AnimatorSetAnimation( &_sprite, SANTA_STATE_MOVE_RIGHT );
                    OutputDebugString( "Heading Back!\n" );
                } else {
                    // We're currently back at home sleeping, lets see if the sack is full yet.
                    MonitorEnter( &g_santaMonitor );
                    float weight, volume;
                    GetSackMetrics( &weight, &volume );
                    
                    // See if the stack is full.
                    if ( weight >= MAX_SACK_WEIGHT || volume >= MAX_SACK_VOLUME || _sackIndex >= MAX_SACK_ITEMS ) {
                        _currentRoute = SANTA_ROUTE_DELIVER;
                        AnimatorSetAnimation( &_sprite, SANTA_STATE_MOVE_LEFT_BAG );
                        OutputDebugString( "Time to deliver presents!\n" );
                        _sackSprite.visible = FALSE;
                    } else {
                        float maxPercent = max( weight / (float)MAX_SACK_WEIGHT, max( volume / (float)MAX_SACK_VOLUME, _sackIndex / (float)MAX_SACK_ITEMS ) );

                        if ( maxPercent >= 0.75f ) {
                            _sackSprite.clip = _SackClips[3];
                        } else if ( maxPercent >= 0.5f ) {
                            _sackSprite.clip = _SackClips[2];
                        } else if ( maxPercent >= 0.25f ) {
                            _sackSprite.clip = _SackClips[1];
                        } else {
                            _sackSprite.clip = _SackClips[0];
                        }

                        
                    }
                    MonitorLeave( &g_santaMonitor );
                    GraphicsUpdateSprite( _sackHandle, &_sackSprite );
                }
            } else if ( _currentRoute != SANTA_ROUTE_WAIT ) {
                _previousRoute = _currentRoute;
                
                // We just went back, lets signal the gnomes that the sack is ready again!
                if ( _currentRoute == SANTA_ROUTE_BACK ) {
                    MonitorEnter( &g_santaMonitor );
                    _sackIndex = 0;
                    MonitorSignalAll( &g_santaMonitor );
                    MonitorLeave( &g_santaMonitor );

                    _sackSprite.visible = TRUE;
                    GraphicsUpdateSprite( _sackHandle, &_sackSprite );
                }

                _currentRoute = SANTA_ROUTE_WAIT;
                OutputDebugString( "Gonna wait some..!\n" );
                AnimatorSetAnimation( &_sprite, SANTA_STATE_ASLEEP );
                _waitTimer = 0.0f;
            }

            if ( _currentRoute != SANTA_ROUTE_WAIT ) {
                _sprite.nextPoint = 0;
                _sprite.movementSequence = _Routes[_currentRoute];
            }
        }

        animTimer += DeltaTime;

        lastTick = tick;
    }

    return 0;
}

