//
// Created by Michal Němec on 02/01/2020.
//

#include "lifecycle.h"

#ifdef MKS_LIFECYCLE_ENABLE

std::string mks::lifecycle_state_str(lifecycle_state state) {
    switch(state) {
        case mks::lifecycle_state::created:
            return "created";
        case mks::lifecycle_state::moved:
            return "moved";
        case mks::lifecycle_state::copied:
            return "copied";
        case mks::lifecycle_state::destructed:
            return "destructed";
    }
    return "unknown";
}

#endif