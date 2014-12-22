#include "graphics.h"
#include <assert.h>

#define MAX_SPRITES 256

// Backbuffer to render to.
static HBITMAP hBackBuffer = NULL;
static int bitmapWidth, bitmapHeight;

typedef struct queueEntry_s {
    sprite_t sprite;
    int valid;
} queueEntry_t;

// RenderQueue
static queueEntry_t queue[MAX_SPRITES];

// DeviceContext for target window.
static HDC hDeviceContext = NULL;

// For synchronized drawing.
static CRITICAL_SECTION csQueueAccess;

void GraphicsInit( HDC hdc ) {
    hDeviceContext = hdc;

    memset( queue, 0, sizeof( queue ) );

    HWND hTargetWindow = WindowFromDC( hdc );
    RECT windowSize;
    GetClientRect( hTargetWindow, &windowSize );

    bitmapWidth = windowSize.right - windowSize.left;
    bitmapHeight = windowSize.bottom - windowSize.top;


    BITMAPINFOHEADER bInfoHeader;

    bInfoHeader.biSize = sizeof bInfoHeader;
    bInfoHeader.biBitCount = 32;
    bInfoHeader.biWidth = bitmapWidth;
    bInfoHeader.biHeight = bitmapHeight;
    bInfoHeader.biPlanes = 1;
    bInfoHeader.biSizeImage = 4 * bitmapWidth * bitmapHeight;
    bInfoHeader.biCompression = BI_RGB;

    BITMAPINFO bInfo;

    hBackBuffer = CreateDIBitmap( hDeviceContext, &bInfoHeader, CBM_INIT, NULL, &bInfo, DIB_RGB_COLORS );

    InitializeCriticalSection( &csQueueAccess );

    // hBackBuffer = CreateCompatibleBitmap( hDeviceContext, bitmapWidth, bitmapHeight );
}

void GraphicsShutdown() {

}

ghandle_t GraphicsRegisterSprite( const sprite_t *sprite ) {
    ghandle_t ret = 0;

    EnterCriticalSection( &csQueueAccess );
    for ( size_t i = 0; i < MAX_SPRITES; i++ ) {
        if ( queue[i].valid == 0 ) {
            queue[i].valid = TRUE;
            ret = i;
            break;
        }
    }
    LeaveCriticalSection( &csQueueAccess );

    GraphicsUpdateSprite( ret, sprite );

    return ret;
}

void GraphicsUpdateSprite( ghandle_t handle, const sprite_t *sprite ) {
    EnterCriticalSection( &csQueueAccess );
    assert( handle < MAX_SPRITES && queue[handle].valid );
    queue[handle].sprite = *sprite;
    LeaveCriticalSection( &csQueueAccess );
}

void GraphicsUnRegisterSprite( ghandle_t handle ) {
    EnterCriticalSection( &csQueueAccess );
    assert( handle < MAX_SPRITES && queue[handle].valid );
    queue[handle].valid = FALSE;
    LeaveCriticalSection( &csQueueAccess );
}

void GraphicsProcessQueue() {

    // Create a device context for the backbuffer, and select the backbuffer for it.
    HDC hBackBufferDC = CreateCompatibleDC( hDeviceContext );
    SelectObject( hBackBufferDC, hBackBuffer );

    RECT r ={ 0, 0, bitmapWidth, bitmapHeight };
    FillRect( hBackBufferDC, &r, GetStockObject( GRAY_BRUSH ) );

    // Track the currently bound bitmap.
    HBITMAP hCurrent = NULL;
    HDC hSpriteDC = NULL;

    EnterCriticalSection( &csQueueAccess );

    for ( size_t i = 0; i < MAX_SPRITES; i++ ) {
        if ( queue[i].valid == FALSE || queue[i].sprite.visible == FALSE ) continue;

        const sprite_t *sprite = &queue[i].sprite;

        // Small optimization to avoid binding the same bitmap twice.
        if ( sprite->bitmap != hCurrent ) {
            hCurrent = sprite->bitmap;
            
            if ( hSpriteDC ) {
                DeleteDC( hSpriteDC );
            }

            hSpriteDC = CreateCompatibleDC( hDeviceContext );

            // We're not responsible for destroying any previous bitmaps,
            // that should be managed by whoever created it.
            SelectObject( hSpriteDC, hCurrent );
        }


        int destX = (int)sprite->x;
        int destY = (int)sprite->y;

        int spriteWidth = (int)( ( sprite->clip.right - sprite->clip.left ) * sprite->scaleX );
        int spriteHeight = (int)( ( sprite->clip.bottom - sprite->clip.top ) * sprite->scaleY );
        
        TransparentBlt( hBackBufferDC,
            // Destination
            destX,
            destY,
            spriteWidth,
            spriteHeight,

            // Source
            hSpriteDC,
            sprite->clip.left,
            sprite->clip.top,
            sprite->clip.right - sprite->clip.left,
            sprite->clip.bottom - sprite->clip.top,
            sprite->colorKey );
    }

    LeaveCriticalSection( &csQueueAccess );

    // Draw backbuffer to screen.
    BitBlt( hDeviceContext, 0, 0, bitmapWidth, bitmapHeight, hBackBufferDC, 0, 0, SRCCOPY );

    DeleteDC( hBackBufferDC );
    
    if ( hSpriteDC ) {
        DeleteDC( hSpriteDC );
    }
}