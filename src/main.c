#include <Windows.h>
#include <stdbool.h>
#include <crtdbg.h>

#include "santa.h"
#include "toyfactory.h"
#include "graphics.h"
#include "monitor.h"
#include "..\resource.h"

#define WINDOW_STYLES (WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION)
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

BOOL g_Running;

LRESULT CALLBACK WindowProc( HWND, UINT, WPARAM, LPARAM );
static unsigned int AudioFunc( void* ptr );

static HBITMAP hSpriteSheet = NULL;
static HBITMAP hBackground = NULL;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR cmd, int cmdShow ) {

    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

    InitializeMonitor();

    WNDCLASS wClass;
    memset( &wClass, 0, sizeof( wClass ) );

    wClass.style = CS_OWNDC;
    wClass.lpfnWndProc = WindowProc;
    wClass.hInstance = hInstance;
    wClass.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_SANTA ) );
    wClass.hCursor = LoadCursor( NULL, IDC_ARROW );
    wClass.lpszClassName = TEXT( "SANTA_WNDCLASS" );
    wClass.lpszMenuName = MAKEINTRESOURCE( IDR_MENU1 );

    RegisterClass( &wClass );

    // Calculate the right size for the window, with out desired resolution
    RECT wRect ={ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    AdjustWindowRect( &wRect, WINDOW_STYLES, TRUE );

    HWND hWindow = CreateWindowEx(
        0,
        wClass.lpszClassName,
        TEXT( "A (merry) Thread Visualizer" ),
        WINDOW_STYLES | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wRect.right - wRect.left,
        wRect.bottom - wRect.top,
        NULL,
        NULL,
        hInstance,
        NULL
        );

    MSG msg ={ 0 };

    sprite_t test;
    test.bitmap = hBackground;
    test.scaleX = 1.0f;
    test.scaleY = 1.0f;
    test.clip.left = 0;
    test.clip.right = 640;
    test.clip.top = 0;
    test.clip.bottom = 480;
    test.x = 0.0f;
    test.y = 0.0f;
    test.visible = TRUE;

    GraphicsInit( GetDC( hWindow ) ); // CS_OWNDC means no leaks here.

    ghandle_t h1 = GraphicsRegisterSprite( &test );
    GraphicsUpdateSprite( h1, &test );

    SantaStart( hSpriteSheet );
    ToyFactoryStart( hSpriteSheet, 500, 0 );
    ToyFactoryStart( hSpriteSheet, 500, 288 );

    g_Running = true;

    // TODO(Peter): Enable to make people insane.
    _beginthreadex(
        NULL,
        0,
        AudioFunc,
        NULL,
        NULL );

    while ( g_Running ) {
        // Handle Windows messages.
        while ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) ) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        GraphicsProcessQueue();
    }

    ShutdownMonitor();

    return 0;
}


LRESULT CALLBACK WindowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
    static HDC hdc;

    switch ( msg ) {
    case WM_CREATE:
        hSpriteSheet = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_SANTA ) );
        hBackground = LoadBitmap( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BACKGROUND ) );
        hdc = GetDC( hwnd );
        break;
    case WM_DESTROY:
        SantaStop();
        g_Running = FALSE;
        GraphicsShutdown();
        break;
    }

    return DefWindowProc( hwnd, msg, wParam, lParam );
}

static unsigned int AudioFunc( void* ptr ) {
    while ( g_Running ) {
        PlaySound( MAKEINTRESOURCE( IDR_JINGLE ), GetModuleHandle( NULL ), SND_RESOURCE );
    }
    return 0;
}
