#pragma once

typedef enum {
    PED_NONE = 0,
    PED_NS = 1,
    PED_EW = 2
} PedestrianDirection;

#define PED_VEHICLE_YELLOW_SECONDS 3
#define PED_CROSSING_SECONDS 20
#define PED_BUTTON_COOLDOWN_SECONDS 60

int PedestrianRequest(PedestrianDirection direction, int train_active);
int PedestrianPending(PedestrianDirection direction);
int PedestrianTakePending(PedestrianDirection direction, int train_active);
void PedestrianService(PedestrianDirection direction);
