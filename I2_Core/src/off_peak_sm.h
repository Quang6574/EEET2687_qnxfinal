// off_peak_sm.h
#pragma once


int OffPeakSMStart(enum states *CurrentState);

// Global variable as killFlag (1 = keep alive || 0 = kill)
extern int living;
// Lock flag for train detection (0 = no train || 1 = train)
extern int isLocked_OFF;
