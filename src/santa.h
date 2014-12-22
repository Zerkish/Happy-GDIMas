#ifndef SANTA_H
#define SANTA_H
#include "toyfactory.h"
#include <Windows.h>

#define SANTA_SACK_LOCATION_X 432
#define SANTA_SACK_LOCATION_Y 240

extern int g_santaMonitor;

// Starts the Santa thread.
void SantaStart( HBITMAP spriteSheet );

// Stops the Santa thread.
void SantaStop();

// Puts a toy in santas sack.
// Returns non-zero if the sack is available, and not full.
void SantaPutToy( toy_t );



#endif // !SANTA_H
