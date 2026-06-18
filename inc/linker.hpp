#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

enum class LinkRelocationType {
    ABS32,
    DISP12
};

struct LinkSection {
    std::string name;
    std::vector<uint8_t> data;
};

struct LinkSymbol {
    std::string name;
    std::string section;
    uint32_t value;
    bool global;
    bool defined;
    bool external;
};

struct LinkRelocation {
    std::string section;
    uint32_t offset;
    std::string symbol;
    LinkRelocationType type;
};

struct ObjectFile {
    std::vector<LinkSection> sections;
    std::vector<LinkSymbol> symbols;
    std::vector<LinkRelocation> relocations;
};

struct SectionPlacement {

    int objectIndex;

    std::string sectionName;

    uint32_t offset;

};

struct GlobalSymbol {

    std::string name;

    std::string section;

    uint32_t value;

    bool global;

};

struct GlobalRelocation {
    std::string section;
    uint32_t offset;
    std::string symbol;
    LinkRelocationType type;
};

struct ResolvedSymbol {
    std::string name;
    uint32_t address;
};

class Linker {
public:
    ObjectFile readObjectFile(const std::string& filename);

    void printObjectFile(const ObjectFile& obj);

    void addObjectFile(const ObjectFile& obj);

    void mergeSections();

    void buildGlobalSymbolTable();

    void buildGlobalRelocationTable();
    
    void assignSectionAddresses(const std::vector<std::pair<std::string, uint32_t>>& placeOptions);

    void resolveSymbols();

    void resolveRelocations();

    void writeHexFile(const std::string& filename);

    void checkSectionOverlaps();

    void checkUndefinedSymbols();

    void writeRelocatableObject(const std::string& filename);

private:

    uint32_t findResolvedSymbolAddress(const std::string& name);

    LinkSection* findMergedSection(const std::string& name);

    void write32(LinkSection& section, uint32_t offset, uint32_t value);

    uint32_t read32(const LinkSection& section, uint32_t offset);

    std::vector<GlobalSymbol> globalSymbols;

    std::vector<ObjectFile> objectFiles;

    std::vector<LinkSection> mergedSections;

    std::vector<SectionPlacement> placements;

    std::vector<GlobalRelocation> globalRelocations;

    std::unordered_map<std::string, uint32_t> sectionBases;

    std::vector<ResolvedSymbol> resolvedSymbols;



}; 