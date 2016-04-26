#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace utils {
    template <typename T, int N>
    int length(const T (&) [N]) { return N; }

    std::string getExePath();

    std::vector<char> loadAsset(const std::string &filename);
    std::vector<unsigned char> loadPng(const char *filename, unsigned int *width, unsigned int *height, unsigned int *bpp);

    void printStack( void );

    extern glm::mat4 biasMatrix;
}
