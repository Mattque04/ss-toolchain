#include "../inc/linker.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <stdexcept>

static uint32_t parseNumber(const std::string& s)
{
    return std::stoul(s, nullptr, 0);
}

int main(int argc, char** argv)
{
    try {
        bool hexMode = false;
        bool relocatableMode = false;

        std::string outputFile;
        std::vector<std::string> inputFiles;
        std::vector<std::pair<std::string, uint32_t>> placeOptions;

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "-hex") {
                hexMode = true;
            }
            else if (arg == "-relocatable") {
                relocatableMode = true;
            }
            else if (arg == "-o") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("Missing output file after -o");
                }
                outputFile = argv[++i];
            }
            else if (arg.rfind("-place=", 0) == 0) {
                std::string value = arg.substr(7);
                size_t at = value.find('@');

                if (at == std::string::npos) {
                    throw std::runtime_error("Invalid -place option: " + arg);
                }

                std::string sectionName = value.substr(0, at);
                uint32_t address = parseNumber(value.substr(at + 1));

                placeOptions.push_back({sectionName, address});
            }
            else {
                inputFiles.push_back(arg);
            }
        }

        if (hexMode == relocatableMode) {
            throw std::runtime_error("Exactly one of -hex or -relocatable must be specified");
        }

        if (outputFile.empty()) {
            throw std::runtime_error("Missing -o output file");
        }

        if (inputFiles.empty()) {
            throw std::runtime_error("No input object files");
        }

        Linker linker;

        for (const auto& file : inputFiles) {
            ObjectFile obj = linker.readObjectFile(file);
            linker.addObjectFile(obj);
        }

        linker.mergeSections();
        linker.buildGlobalSymbolTable();
        linker.buildGlobalRelocationTable();

        if (hexMode) {
            linker.assignSectionAddresses(placeOptions);
            linker.checkSectionOverlaps();
            linker.resolveSymbols();
            linker.checkUndefinedSymbols();
            linker.resolveRelocations();
            linker.writeHexFile(outputFile);
        }
        else {
            linker.writeRelocatableObject(outputFile);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Linker error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}