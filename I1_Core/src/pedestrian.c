#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "pedestrian.h"

static pthread_mutex_t pedestrian_mutex = PTHREAD_MUTEX_INITIALIZER;
static PedestrianDirection pending_direction = PED_NONE;
static time_t last_request_time;

int PedestrianRequest(PedestrianDirection direction, int train_active)
{
    time_t now;
    int accepted = 0;

    if (direction != PED_NS && direction != PED_EW) {
        return 0;
    }

    pthread_mutex_lock(&pedestrian_mutex);
    now = time(NULL);
    if (!train_active && pending_direction == PED_NONE &&
        (last_request_time == 0 ||
         difftime(now, last_request_time) >= PED_BUTTON_COOLDOWN_SECONDS)) {
        pending_direction = direction;
        last_request_time = now;
        accepted = 1;
    }
    pthread_mutex_unlock(&pedestrian_mutex);

    if (accepted) {
        printf("Pedestrian %s request accepted.\n",
               direction == PED_NS ? "NS" : "EW");
    } else {
        printf("Pedestrian request rejected: train active, crossing busy, or cooldown active.\n");
    }
    return accepted;
}

int PedestrianPending(PedestrianDirection direction)
{
    int pending;

    pthread_mutex_lock(&pedestrian_mutex);
    pending = pending_direction == direction;
    pthread_mutex_unlock(&pedestrian_mutex);
    return pending;
}

int PedestrianTakePending(PedestrianDirection direction, int train_active)
{
    int accepted = 0;

    pthread_mutex_lock(&pedestrian_mutex);
    if (pending_direction == direction) {
        if (train_active) {
            pending_direction = PED_NONE;
        } else {
            pending_direction = PED_NONE;
            accepted = 1;
        }
    }
    pthread_mutex_unlock(&pedestrian_mutex);
    return accepted;
}

void PedestrianService(PedestrianDirection direction)
{
    if (direction == PED_NS) {
        printf("NS pedestrian request: EW vehicle yellow for %d seconds.\n",
               PED_VEHICLE_YELLOW_SECONDS);
        sleep(PED_VEHICLE_YELLOW_SECONDS);
        printf("NS pedestrian light green; EW pedestrian light red for %d seconds.\n",
               PED_CROSSING_SECONDS);
    } else if (direction == PED_EW) {
        printf("EW pedestrian request: NS vehicle yellow for %d seconds.\n",
               PED_VEHICLE_YELLOW_SECONDS);
        sleep(PED_VEHICLE_YELLOW_SECONDS);
        printf("EW pedestrian light green; NS pedestrian light red for %d seconds.\n",
               PED_CROSSING_SECONDS);
    } else {
        return;
    }

    sleep(PED_CROSSING_SECONDS);
    printf("Pedestrian crossing complete; both pedestrian lights red.\n");
}
