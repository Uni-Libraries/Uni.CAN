//
// Includes
//

// app
#include "app_state.h"



//
// Implementation
//

namespace APP {
    CanManager & State::CanMgr() {
        return m_can_mgr;
    }
}
