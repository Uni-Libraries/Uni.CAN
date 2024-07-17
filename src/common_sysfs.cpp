// stdlib

#include <fstream>

#if defined(__GNUC__) && __GNUC__ < 8
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif


// uni.can
#include "common_sysfs.h"

using namespace std::string_literals;


namespace Auris::CAN {
    //
    // Public
    //

    bool SysFs::IsClassExists(const std::string &className, const std::string &elementName) {
        return fs::exists("/sys/class/"s + className + "/" + elementName + "/");
    }

    int SysFs::GetClassProperty(const std::string &className, const std::string &elementName,
                                const std::string &propertyName) {
        auto filepath = "/sys/class/"s + className + "/" + elementName + "/" + propertyName;
        if (fs::exists(filepath)) {
            std::ifstream file(filepath);
            std::string file_content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

            return std::stoi(file_content);
        }

        return -1;
    }
} // namespace Auris::CAN
