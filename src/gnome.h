#ifndef GNOME_H
#define GNOME_H
#include "toyfactory.h"
#include <Windows.h>

// Sends a gnome to santa with a toy.
void GnomeStart( HBITMAP sprite, const POINT *path, int steps, toy_t toy );

#endif // !GNOME_H