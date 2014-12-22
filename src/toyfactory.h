#ifndef TOYFACTORY_H
#define TOYFACTORY_H
#include <Windows.h>

#define MAX_TOYNAME_LENGTH 32

typedef struct toy_s {
    char name[MAX_TOYNAME_LENGTH];
    float weight;
    float volume;
} toy_t;

// Starts a toyfactory at position (x, y)
void ToyFactoryStart( HBITMAP sprite, int x, int y );

// Stops all started toyfactories.
void ToyFactoryStopAll();

#endif // !TOYFACTORY_H