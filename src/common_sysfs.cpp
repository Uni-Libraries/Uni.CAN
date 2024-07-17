// stdlib
#include <filesystem>
#include <fstream>

// auris.can --> private
#include "common_sysfs.h"

using namespace std::string_literals;



namespace Auris::CAN {
    //
    // Public
    //

    bool SysFs::IsClassExists(const std::string &className, const std::string &elementName) {
        return std::filesystem::exists("/sys/class/"s + className + "/" + elementName + "/");
    }

    int SysFs::GetClassProperty(const std::string &className, const std::string &elementName, const std::string &propertyName) {
        auto filepath = "/sys/class/"s + className + "/" + elementName + "/" + propertyName;
        if (std::filesystem::exists(filepath)) {
            std::ifstream file(filepath);
            std::string file_content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

            return std::stoi(file_content);
        }

        return -1;
    }
} // namespace Auris::CAN
