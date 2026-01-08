#pragma once

//
// Includes
//

// stdlib
#include <string_view>



//
// Classes
//

namespace APP {
    enum class CanManagerState {
        STANDBY,
        CAN_ERROR,
        CONNECTING,
        OPERATING,
    };

    std::string_view to_string(CanManagerState state);
}
