// on_peak_sm.h
#pragma once


int OnPeakSMStart(enum states currentState);

// Lock flag for train detection (0 = no train || 1 = train)
extern int isLocked_ON;
