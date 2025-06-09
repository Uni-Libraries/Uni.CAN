#pragma once

// stdlib
#include <string>

//
// SysFs
//

namespace Uni::CAN {
    class SysFs {
    public:
        static bool IsClassExists(const std::string &className, const std::string &elementName);

        static int GetClassProperty(const std::string &className, const std::string &elementName,
                                    const std::string &propertyName);
    };
} // namespace Uni::CAN
