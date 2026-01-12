//
// Includes
//

// Uni.GUI
#include "ui_callbacks.h"

// stdlib
#include <memory>
#include <vector>
// Windows SDK
#if defined(_WIN32)
#include <windows.h>
#endif

// Uni.GUI
#include "ui_app.h"

// APP
#include "window_can_connect.h"
#include "window_can_rx.h"
#include "window_can_tx.h"



//
// Global
//

APP::State g_state;



//
// Implementation
//

std::string uni_gui_app_name_get()
{
   return "Uni.CAN App";
}

std::string uni_gui_app_version_get()
{
   return "1.0.3";
}

std::vector<std::shared_ptr<Uni::GUI::UiElement>> uni_gui_app_initialize(int argc, char **argv)
{
   // register windows
   return {
      std::make_shared<APP::WindowCanConnect>(g_state),
      std::make_shared<APP::WindowCanRx>(g_state),
      std::make_shared<APP::WindowCanTx>(g_state),
   };
}
