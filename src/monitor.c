#include "monitor.h"
#include <assert.h>
#include <Windows.h>

typedef struct lock_s {
    void *ptr;
    CRITICAL_SECTION *criticalSection;
    CONDITION_VARIABLE *cond;
} lock_t;


static lock_t *_locks;
static int _numLocks;
static CRITICAL_SECTION _csLocks;
static int _doInit = TRUE;

__inline lock_t *FindLock( void *ptr ) {
    for ( int i = 0; i < _numLocks; ++i ) {
        if ( _locks[i].ptr == ptr ) {
            return &_locks[i];
        }
    }
    return NULL;
}

void InitializeMonitor() {
    if ( _doInit ) {
        InitializeCriticalSection( &_csLocks );
        _doInit = FALSE;
    }
}

void ShutdownMonitor() {
    for ( int i = 0; i < _numLocks; ++i ) {
        free( _locks[i].criticalSection );
        free( _locks[i].cond );
    }
    free( _locks );
    _locks = NULL;
}

void MonitorEnter( void *object ) {
    EnterCriticalSection( &_csLocks );
    //OutputDebugString( "Entering monitor\n" );

    lock_t *lock = FindLock( object );

    if ( lock == NULL ) {
        if ( _locks == NULL ) {
            _locks = malloc( sizeof( lock_t ) );
        } else {
            _locks = realloc( _locks, ( _numLocks + 1 ) * sizeof( lock_t ) );
        }

        lock = &_locks[_numLocks];
        ++_numLocks;

        lock->criticalSection = malloc( sizeof( CRITICAL_SECTION ) );
        lock->cond = malloc( sizeof( CONDITION_VARIABLE ) );

        InitializeCriticalSection( lock->criticalSection );
        InitializeConditionVariable( lock->cond );
        lock->ptr = object;
    }

    CRITICAL_SECTION *cs = lock->criticalSection;
    LeaveCriticalSection( &_csLocks );

    EnterCriticalSection( cs );
}

void MonitorLeave( void *object ) {
    //OutputDebugString( "Leaving monitor\n" );
    EnterCriticalSection( &_csLocks );
    lock_t *lock = FindLock( object );
    assert( lock && "Can't leave a monitor that doesn't exist!" );
    CRITICAL_SECTION *cs = lock->criticalSection;
    LeaveCriticalSection( &_csLocks );
    
    LeaveCriticalSection( cs );
}

void MonitorWait( void *object ) {
    EnterCriticalSection( &_csLocks );
    lock_t *lock = FindLock( object );
    assert( lock && "Can't leave a monitor that doesn't exist!" );
    CONDITION_VARIABLE *cond = lock->cond;
    CRITICAL_SECTION *cs = lock->criticalSection;
    LeaveCriticalSection( &_csLocks );

    // This is the basic monitor concept, we release the "mutex"
    // Sleep on the condition variable, and when we awake, we try to grab the mutex again.
    OutputDebugString( "Waiting in monitor\n" );
    assert( SleepConditionVariableCS( cond, cs, INFINITE ) );
    OutputDebugString( "Woke up in monitor\n" );
}

void MonitorSignal( void *object ) {
    EnterCriticalSection( &_csLocks );
    lock_t *lock = FindLock( object );
    assert( lock && "Can't leave a monitor that doesn't exist!" );
    CONDITION_VARIABLE *cond = lock->cond;
    CRITICAL_SECTION *cs = lock->criticalSection;
    LeaveCriticalSection( &_csLocks );

    // This will wake up one thread waiting on the variable.
    // It will instantly try to grab the mutex again.
    OutputDebugString( "Signal monitor\n" );
    WakeConditionVariable( cond );
}

void MonitorSignalAll( void *object ) {
    EnterCriticalSection( &_csLocks );
    lock_t *lock = FindLock( object );
    assert( lock && "Can't leave a monitor that doesn't exist!" );
    CONDITION_VARIABLE *cond = lock->cond;
    CRITICAL_SECTION *cs = lock->criticalSection;
    LeaveCriticalSection( &_csLocks );

    // This will wake up one thread waiting on the variable.
    // It will instantly try to grab the mutex again.
    OutputDebugString( "Signal monitor\n" );
    WakeAllConditionVariable( cond );
}
