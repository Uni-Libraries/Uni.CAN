#pragma once

//
// Includes
//

// app
#include "can_mgr.h"



//
// Typedefs
//

namespace APP {
    class State {
      public:
        CanManager& CanMgr();

      private:
        CanManager m_can_mgr;
    };
}