#pragma once

#include <string>

struct Symbol {
    std::string name;

    int sectionId;
    uint32_t value;

    bool global;
    bool defined;
    bool external;
    bool absolute;
    Symbol(const std::string& name = "", int sectionId = -1, uint32_t value = 0)
        : name(name),
          sectionId(sectionId),
          value(value),
          global(false),
          defined(false),
          external(false),
          absolute(false)
    {}
};