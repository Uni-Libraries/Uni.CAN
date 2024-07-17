#pragma once

// stdlib
#include <string>

//
// SysFs
//

namespace Auris::CAN {
    class SysFs {
      public:
        static bool IsClassExists(const std::string &className, const std::string &elementName);

        static int GetClassProperty(const std::string &className, const std::string &elementName, const std::string &propertyName);
    };
} // namespace Auris::CAN