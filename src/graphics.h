#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Windows.h>

typedef struct sprite_s {

    // Graphics Data
    HBITMAP bitmap;
    COLORREF colorKey;

    // Clip
    RECT clip;

    // Position Data
    float x;
    float y;

    float scaleX;
    float scaleY;

    int visible;

} sprite_t;

typedef size_t ghandle_t;

// Initializes software renderer
void GraphicsInit( HDC hdc );

// Cleans up resources
void GraphicsShutdown();

// Threadsafe
ghandle_t GraphicsRegisterSprite( const sprite_t *sprite );

// Threadsafe
void GraphicsUpdateSprite( ghandle_t handle, const sprite_t *sprite );

// Threadsafe
void GraphicsUnRegisterSprite( ghandle_t handle );

// Processes all sprites in the queue.
void GraphicsProcessQueue();

#endif // !GRAPHICS_H
