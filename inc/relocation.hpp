#pragma once

#include <string>
#include <cstdint>

enum class RelocationType {
    ABS32,
    DISP12
};

struct Relocation {
    int sectionId;
    uint32_t offset;
    std::string symbol;
    RelocationType type;

    Relocation(
        int sectionId = -1,
        uint32_t offset = 0,
        const std::string& symbol = "",
        RelocationType type = RelocationType::ABS32
    )
        : sectionId(sectionId),
          offset(offset),
          symbol(symbol),
          type(type)
    {}
};