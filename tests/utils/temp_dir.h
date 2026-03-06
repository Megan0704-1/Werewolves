#pragma once

#include <filesystem>
#include <sstream>
#include <string>
#include <unistd.h>

namespace testutils {
namespace fs = std::filesystem;
class TempDir {
    public:
        TempDir() {
            std::stringstream ss;
            ss << "/tmp/werewolf_test_" << getpid() << "_" << rand();
            path_ = ss.str();

            fs::remove_all(path_);
            fs::create_directories(path_);
        }

        ~TempDir() {
            fs::remove_all(path_);
        }

        const std::string& path() const {
            return path_;
        }

    private:
        std::string path_;
};

} // namespace testutils

