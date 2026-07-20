#pragma once

#include <string>
#include <cstdint>

struct Relocation {
    int sectionId;
    uint32_t offset;
    std::string symbol;

    Relocation(int sectionId = -1, uint32_t offset = 0, const std::string& symbol = "")
        : sectionId(sectionId),
          offset(offset),
          symbol(symbol)
    {}
};