#ifndef MONITOR_H
#define MONITOR_H

// TODO(Peter): Implement a monitor synchronization structure.

// Call these once per application..
void InitializeMonitor();
void ShutdownMonitor();

void MonitorEnter( void *lockObject );
void MonitorLeave( void *lockObject );
void MonitorWait( void *lockObject );
void MonitorSignal( void *lockObject );
void MonitorSignalAll( void *lockObject );

#endif // !MONITOR_H