//
// Includes
//

// magic_enum
#include <magic_enum/magic_enum.hpp>

// app
#include "can_mgr_state.h"



//
// Implementation
//

namespace APP {
    std::string_view to_string(CanManagerState state) {
        return magic_enum::enum_name(state);
    }
}