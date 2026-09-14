#pragma once

#include <stdio.h>


//TODO CHANGE ALL THESES FUNCTION IN ORDER TO LOG IN A FILE

#define ERR_LOG(target, dscrp) \
        printf("[ERROR] %s : %s\n", target, dscrp);

#define SUCC_LOG(target, dscrp) \
        printf("[SUCCESS] %s : %s\n", target, dscrp);

#define WARN_LOG(target, dscrp) \
        printf("[WARN] %s : %s\n", target, dscrp);

#define DEBUG_LOG(target, dscrp) \
        printf("[DEBUG] %s : %s\n", target, dscrp);

#define RESET_COLOR printf("\033[0m")
