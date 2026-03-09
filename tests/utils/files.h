#pragma once

#include <fstream>
#include <string>

namespace testutils {

std::string ReadFileContents(const std::string& path) {
    std::ifstream ifs(path);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

} // namespace testutils

