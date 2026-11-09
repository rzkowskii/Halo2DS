#ifndef HALO_N3DS_ENGINE_STATE_PATH_H
#define HALO_N3DS_ENGINE_STATE_PATH_H

#ifdef HALO_N3DS_STARTUP_DIAGNOSTICS
#define N3DS_STATE_ROOT "sdmc:/halo-source/diagnostic-state"
#define N3DS_STALL_LOG "sdmc:/halo-startup-diagnostics-stall.log"
#else
#define N3DS_STATE_ROOT "sdmc:/halo-source/state"
#define N3DS_STALL_LOG "sdmc:/halo-source-stall.log"
#endif

#endif
