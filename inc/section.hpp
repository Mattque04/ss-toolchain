#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct Section {
    std::string name;

    std::vector<uint8_t> data;

    Section(const std::string& name = "")
        : name(name)
    {}
};