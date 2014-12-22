#include "toyfactory.h"
#include "graphics.h"
#include "animator.h"
#include "gnome.h"
#include <process.h>
#include <Windows.h>
#include <stdlib.h>

#define MAX_FACTORIES 2

// 25 toys, as requested..... (Couldn't this just have been 25 ints?).
static const toy_t _ToyTemplates[] ={
        { "Furble", 1.0f, 0.25f },
        { "Teddybear", 0.125f, 0.5f },
        { "Doll", 0.2f, 0.4f },
        { "Baseball", 0.1f, 0.1f },
        { "Baseball Glove", 0.3f, 0.2f },
        { "Skateboard", 2.0f, 1.5f },
        { "Rollerblades", 4.0f, 2.0f },
        { "Wooden Horse", 0.3f, 0.5f },
        { "Book", 0.2f, 0.4f },
        { "Keyboard", 0.4f, 0.8f },
        { "Soccer Ball", 0.35f, 0.4f },
        { "Newton's Cradle", 0.5f, 0.8f },
        { "Monopoly Game", 1.0f, 1.0f },
        { "Play-doh", 1.0f, 0.25f },
        { "Lego", 0.5f, 1.0f },
        { "Walkie-Talkie set", 0.8f, 1.0f },
        { "Toy Car", 0.25f, 0.4f },
        { "Barbie Doll", 0.25f, 0.4f },
        { "Action Man", 0.3f, 0.5f },
        { "Toy Gun", 0.5f, 0.75f },
        { "Meccano", 2.0f, 1.0f },
        { "Gameboy", 1.0f, 0.5f },
        { "Skipping Rope", 0.2f, 0.2f },
        { "Nerf gun", 0.4f, 0.7f },
        { "FurReal Lion", 1.0f, 1.25f },
        { "Transformer", 0.75f, 0.5f }
};

static unsigned int _stdcall ThreadFunc( void * );

typedef struct toyFactory_s {
    ghandle_t spriteHandle;
    sprite_t sprite;
    uintptr_t threadId;
    int doorOpen;
    float toyProgress;
    float currentProductionTime;
    POINT gnomePath[4];
    sprite_t progressBar;
    sprite_t progressBarOutline;
    int pbHandle;
    int pboHandle;
} toyFactory_t;

static toyFactory_t _factories[MAX_FACTORIES];
static int _factoryIndex;
static int _stopFactories;

static POINT _Paths[MAX_FACTORIES][4] = {
        { { 560, 112 }, { 560, 128 }, { 432, 128 }, { 432, 172 } },
        { { 560, 368 }, { 432, 368 }, { 432, 336 }, { 432, 212 } }
};

void ToyFactoryStart( HBITMAP sprite, int x, int y ) {

    if ( _factoryIndex < MAX_FACTORIES ) {

        toyFactory_t *factory = &_factories[_factoryIndex];

        // Setup rendering for the factory.
        factory->sprite.bitmap = sprite;
        factory->sprite.x = (float)x;
        factory->sprite.y = (float)y;
        factory->sprite.visible = TRUE;
        factory->sprite.colorKey = RGB( 255, 0, 255 );
        factory->sprite.scaleX = 2.0f;
        factory->sprite.scaleY = 2.0f;
        factory->sprite.clip.left = 0;
        factory->sprite.clip.right = 64;
        factory->sprite.clip.top = 192;
        factory->sprite.clip.bottom = 256;
        factory->spriteHandle = GraphicsRegisterSprite( &factory->sprite );
        memcpy( factory->gnomePath, _Paths[_factoryIndex], sizeof( _Paths[0] ) );

        GraphicsUpdateSprite( factory->spriteHandle, &factory->sprite );

        factory->progressBar.bitmap = sprite;
        factory->progressBar.clip.left = 128;
        factory->progressBar.clip.right = 129;
        factory->progressBar.clip.top = 224;
        factory->progressBar.clip.bottom = 225;

        factory->progressBar.scaleX = 0.0f;
        factory->progressBar.scaleY = 6.0f;
        factory->progressBar.x = factory->sprite.x;
        factory->progressBar.y = factory->sprite.y + 4.0f;
        factory->progressBar.visible = TRUE;
        factory->progressBar.colorKey = RGB( 255, 0, 255 );
        
        factory->progressBarOutline = factory->progressBar;
        factory->progressBarOutline.bitmap = sprite;
        factory->progressBarOutline.clip.top = 224;
        factory->progressBarOutline.clip.bottom = 225;
        factory->progressBarOutline.clip.left = 160;
        factory->progressBarOutline.clip.right = 161;
        factory->progressBarOutline.scaleY = 10.0f;
        factory->progressBarOutline.scaleX = 132.0f;
        factory->progressBarOutline.x = factory->sprite.x - 2.0f;
        factory->progressBarOutline.y = factory->sprite.y + 2.0f;
        
        factory->pboHandle = GraphicsRegisterSprite( &factory->progressBarOutline );
        factory->pbHandle = GraphicsRegisterSprite( &factory->progressBar );
        
        GraphicsUpdateSprite( factory->pbHandle, &factory->progressBar );
        GraphicsUpdateSprite( factory->pboHandle, &factory->progressBarOutline );

        // Start thread and send in the factory as a parameter to the thread.
        _factories[_factoryIndex].threadId = _beginthreadex(
            NULL,
            0,
            ThreadFunc,
            factory,
            0,
            NULL );

        ++_factoryIndex;
    }

}

void ToyFactoryStopAll() {
    _stopFactories = TRUE;
}

static unsigned int _stdcall ThreadFunc( void *ptr ) {
    toyFactory_t *factory = (toyFactory_t *)ptr;

    factory->doorOpen = FALSE;
    factory->toyProgress = -1.0f;
    factory->currentProductionTime = 0.0f;

    LARGE_INTEGER tick, lastTick, frequency;
    QueryPerformanceFrequency( &frequency );
    QueryPerformanceCounter( &lastTick );

    float doorTimer = 0.0f;

    while ( !_stopFactories ) {

        QueryPerformanceCounter( &tick );
        const float DeltaTime = (float)( tick.QuadPart - lastTick.QuadPart ) / (float)( frequency.QuadPart );

        if ( factory->toyProgress < 0 ) {
            unsigned int num;
            rand_s( &num );
            num = num & 0xffff;
            factory->currentProductionTime = (float)((num % 100) / 13.0f + 0.25f);
            factory->toyProgress = 0.0f;
            GraphicsUpdateSprite( factory->spriteHandle, &factory->sprite );
        }

        factory->toyProgress += DeltaTime;

        // Are we done producing the toy?
        if ( factory->toyProgress > factory->currentProductionTime ) {
            
            factory->doorOpen = TRUE;
            factory->sprite.clip.left = 64;
            factory->sprite.clip.right = 128;
            GraphicsUpdateSprite( factory->spriteHandle, &factory->sprite );
            factory->toyProgress = -999.0f;
            
            unsigned int num;
            rand_s( &num );
            num = num & 0xffff;
            toy_t toy = _ToyTemplates[num % ARRAYSIZE( _ToyTemplates )];

            GnomeStart( factory->sprite.bitmap, factory->gnomePath, 4, toy );

            //Sleep( 50 );
            doorTimer = 0.0f;
        }

        doorTimer += DeltaTime;

        if ( doorTimer > 0.75f && factory->doorOpen ) {
            factory->doorOpen = FALSE;
            factory->sprite.clip.left = 0;
            factory->sprite.clip.right = 64;
            GraphicsUpdateSprite( factory->spriteHandle, &factory->sprite );
        }

        float progress = factory->toyProgress / factory->currentProductionTime;

        factory->progressBar.scaleX = progress * 128;

        GraphicsUpdateSprite( factory->pbHandle, &factory->progressBar );

        lastTick = tick;

        Sleep( 0 );
    }

    return 0;
}