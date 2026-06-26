#include "../inc/linker.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

static uint8_t parseByte(const std::string& s)
{
    return static_cast<uint8_t>(std::stoul(s, nullptr, 16));
}

ObjectFile Linker::readObjectFile(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in) {
        throw std::runtime_error("Cannot open object file: " + filename);
    }

    ObjectFile obj;
    std::string token;

    in >> token;
    if (token != "#SS_OBJECT") {
        throw std::runtime_error("Invalid object file: " + filename);
    }

    while (in >> token) {
        if (token == "[SECTIONS]") {
            int count;
            in >> count;

            for (int i = 0; i < count; i++) {
                LinkSection sec;
                size_t size;

                in >> sec.name >> size;

                for (size_t j = 0; j < size; j++) {
                    std::string byteStr;
                    in >> byteStr;
                    sec.data.push_back(parseByte(byteStr));
                }

                obj.sections.push_back(sec);
            }
        }
        else if (token == "[SYMBOLS]") {
            int count;
            in >> count;

            for (int i = 0; i < count; i++) {
                int id;
                std::string scope;
                std::string state;

                LinkSymbol sym;

                in >> id >> sym.name >> sym.section >> sym.value >> scope >> state;

                sym.global = (scope == "GLOBAL");
                sym.external = (state == "EXTERN");
                sym.defined = (state == "DEFINED");

                obj.symbols.push_back(sym);
            }
        }
        else if (token == "[RELOCATIONS]") {
            int count;
            in >> count;

            for (int i = 0; i < count; i++) {
                LinkRelocation rel;
                std::string type;

                in >> rel.section >> rel.offset >> rel.symbol >> type;

                rel.type = (type == "ABS32")
                    ? LinkRelocationType::ABS32
                    : LinkRelocationType::DISP12;

                obj.relocations.push_back(rel);
            }
        }
        else if (token == "[END]") {
            break;
        }
    }

    return obj;
}

void Linker::printObjectFile(const ObjectFile& obj)
{
    std::cout << "Sections:\n";
    for (const auto& sec : obj.sections) {
        std::cout << "  " << sec.name << " size=" << sec.data.size() << "\n";
    }

    std::cout << "Symbols:\n";
    for (const auto& sym : obj.symbols) {
        std::cout << "  " << sym.name
                  << " section=" << sym.section
                  << " value=" << sym.value
                  << " global=" << sym.global
                  << " defined=" << sym.defined
                  << " external=" << sym.external
                  << "\n";
    }

    std::cout << "Relocations:\n";
    for (const auto& rel : obj.relocations) {
        std::cout << "  " << rel.section
                  << " offset=" << rel.offset
                  << " symbol=" << rel.symbol
                  << " type=" << (rel.type == LinkRelocationType::ABS32 ? "ABS32" : "DISP12")
                  << "\n";
    }
}

static LinkSection* findSection(std::vector<LinkSection>& sections, const std::string& name)
{
    for (auto& sec : sections) {
        if (sec.name == name) {
            return &sec;
        }
    }
    return nullptr;
}


void Linker::mergeSections()
{
    mergedSections.clear();
    placements.clear();

    for (int objIdx = 0; objIdx < (int)objectFiles.size(); objIdx++) {

        const ObjectFile& obj = objectFiles[objIdx];

        for (const auto& sec : obj.sections) {

            LinkSection* merged = findSection(mergedSections, sec.name);

            if (!merged) {

                LinkSection newSec;
                newSec.name = sec.name;

                mergedSections.push_back(newSec);

                merged = &mergedSections.back();
            }

            uint32_t offset = merged->data.size();

            placements.push_back({
                objIdx,
                sec.name,
                offset
            });
            //ovo bukvalno radi dodaj na kraj merged.data sve iz trenutne sekcije i time prosirujemo merged sekciju
            merged->data.insert(
                merged->data.end(),
                sec.data.begin(),
                sec.data.end()
            );
        }
    }

    std::cout << "\n=== MERGED SECTIONS ===\n";

    for (const auto& sec : mergedSections) {
        std::cout
            << sec.name
            << " size="
            << sec.data.size()
            << "\n";
    }

    std::cout << "\n=== PLACEMENTS ===\n";

    for (const auto& p : placements) {
        std::cout
            << "obj" << p.objectIndex
            << " "
            << p.sectionName
            << " -> "
            << p.offset
            << "\n";
    }
}

void Linker::addObjectFile(const ObjectFile& obj)
{
    objectFiles.push_back(obj);
}

static const SectionPlacement* findPlacement(const std::vector<SectionPlacement>& placements, int objectIndex, const std::string& sectionName)
{
    for (const auto& p : placements) {
        if (p.objectIndex == objectIndex &&
            p.sectionName == sectionName)
        {
            return &p;
        }
    }

    return nullptr;
}

static bool symbolExists(
    const std::vector<GlobalSymbol>& symbols,
    const std::string& name)
{
    for (const auto& s : symbols) {
        if (s.name == name) {
            return true;
        }
    }

    return false;
}

void Linker::buildGlobalSymbolTable()
{
    globalSymbols.clear();

    for (int objIdx = 0; objIdx < (int)objectFiles.size(); objIdx++) {

        const ObjectFile& obj = objectFiles[objIdx];

        for (const auto& sym : obj.symbols) {

            if (!sym.defined) {
                continue;
            }

            if (symbolExists(globalSymbols, sym.name)) {
                throw std::runtime_error(
                    "Multiple definition of symbol: " + sym.name
                );
            }
            
            GlobalSymbol gs;
            if (sym.section == "ABS") {
                gs.name = sym.name;
                gs.section = "ABS";
                gs.value = sym.value;
                gs.global = sym.global;
                globalSymbols.push_back(gs);
                continue;
            }

            const SectionPlacement* placement = findPlacement(placements, objIdx, sym.section);

            if (!placement) {
                throw std::runtime_error(
                    "Missing placement for section: " +
                    sym.section
                );
            }
            gs.name = sym.name;
            gs.section = sym.section;
            gs.value = sym.value + placement->offset;
            gs.global = sym.global;
            globalSymbols.push_back(gs);
        }
    }

    std::cout << "\n=== GLOBAL SYMBOLS ===\n";

    for (const auto& s : globalSymbols) {
        std::cout
            << s.name
            << " section=" << s.section
            << " value=" << s.value
            << " global=" << s.global
            << "\n";
    }
}

void Linker::buildGlobalRelocationTable()
{
    globalRelocations.clear();

    for (int objIdx = 0; objIdx < (int)objectFiles.size(); objIdx++) {

        const ObjectFile& obj = objectFiles[objIdx];

        for (const auto& rel : obj.relocations) {

            const SectionPlacement* placement =
                findPlacement(
                    placements,
                    objIdx,
                    rel.section
                );

            if (!placement) {
                throw std::runtime_error(
                    "Missing placement for section: " +
                    rel.section
                );
            }

            GlobalRelocation gr;

            gr.section = rel.section;
            gr.symbol = rel.symbol;
            gr.type = rel.type;

            gr.offset =
                rel.offset +
                placement->offset;

            globalRelocations.push_back(gr);
        }
    }

    std::cout << "\n=== GLOBAL RELOCATIONS ===\n";

    for (const auto& r : globalRelocations) {

        std::cout
            << r.section
            << " offset=" << r.offset
            << " symbol=" << r.symbol
            << " type="
            << (r.type == LinkRelocationType::ABS32
                    ? "ABS32"
                    : "DISP12")
            << "\n";
    }
}

void Linker::assignSectionAddresses(
    const std::vector<std::pair<std::string, uint32_t>>& placeOptions)
{
    sectionBases.clear();

    for (const auto& p : placeOptions) {
        sectionBases[p.first] = p.second;
    }

    uint32_t nextAddress = 0;

    for (const auto& sec : mergedSections) {
        auto it = sectionBases.find(sec.name);

        if (it != sectionBases.end()) {
            uint32_t end = it->second + sec.data.size();
            if (end > nextAddress) {
                nextAddress = end;
            }
        }
    }

    for (const auto& sec : mergedSections) {
        if (sectionBases.find(sec.name) == sectionBases.end()) {
            sectionBases[sec.name] = nextAddress;
            nextAddress += sec.data.size();
        }
    }

    std::cout << "\n=== SECTION BASES ===\n";

    for (const auto& [name, addr] : sectionBases) {
        std::cout << name << " -> 0x"
                  << std::hex << addr << std::dec << "\n";
    }
}

void Linker::resolveSymbols()
{
    resolvedSymbols.clear();

    for (const auto& sym : globalSymbols)
    {   
        ResolvedSymbol rs;
        if (sym.section == "ABS") {
            rs.name = sym.name;
            rs.address = sym.value;
            resolvedSymbols.push_back(rs);
            continue;
        }
        auto it = sectionBases.find(sym.section);

        if (it == sectionBases.end())
        {
            throw std::runtime_error(
                "No base address for section: " +
                sym.section
            );
        }
        rs.name = sym.name;
        rs.address = it->second + sym.value;

        resolvedSymbols.push_back(rs);
    }

    std::cout << "\n=== RESOLVED SYMBOLS ===\n";

    for (const auto& s : resolvedSymbols)
    {
        std::cout
            << s.name
            << " -> 0x"
            << std::hex
            << s.address
            << std::dec
            << "\n";
    }
}

uint32_t Linker::findResolvedSymbolAddress(
    const std::string& name)
{
    for (const auto& sym : resolvedSymbols)
    {
        if (sym.name == name)
        {
            return sym.address;
        }
    }

    throw std::runtime_error(
        "Unknown symbol: " + name
    );
}

LinkSection* Linker::findMergedSection(
    const std::string& name)
{
    for (auto& sec : mergedSections)
    {
        if (sec.name == name)
        {
            return &sec;
        }
    }

    return nullptr;
}

void Linker::write32(
    LinkSection& section,
    uint32_t offset,
    uint32_t value)
{
    section.data[offset + 0] =
        value & 0xFF;

    section.data[offset + 1] =
        (value >> 8) & 0xFF;

    section.data[offset + 2] =
        (value >> 16) & 0xFF;

    section.data[offset + 3] =
        (value >> 24) & 0xFF;
}

uint32_t Linker::read32(const LinkSection& section, uint32_t offset)
{
    return
        ((uint32_t)section.data[offset + 0]) |
        ((uint32_t)section.data[offset + 1] << 8) |
        ((uint32_t)section.data[offset + 2] << 16) |
        ((uint32_t)section.data[offset + 3] << 24);
}

void Linker::resolveRelocations()
{
    for (const auto& rel : globalRelocations)
    {
        uint32_t symbolAddress = findResolvedSymbolAddress(rel.symbol);

        LinkSection* section = findMergedSection(rel.section);
        if (!section) {
            throw std::runtime_error("Missing merged section: " + rel.section);
        }

        if (rel.type == LinkRelocationType::ABS32)
        {
            write32(*section, rel.offset, symbolAddress);
        }
        else if (rel.type == LinkRelocationType::DISP12)
        {
            uint32_t instrAddress = sectionBases[rel.section] + rel.offset;

            int32_t disp = (int32_t)symbolAddress - ((int32_t)instrAddress+4);

            if (disp < -2048 || disp > 2047) {
                throw std::runtime_error("DISP12 relocation out of range for symbol: " + rel.symbol);
            }

            uint32_t instr = read32(*section, rel.offset);

            instr = (instr & 0xFFFFF000) | (disp & 0xFFF);

            write32(*section, rel.offset, instr);
        }
    }
}

void Linker::writeHexFile(const std::string& filename)
{
    std::ofstream out(filename);
    if (!out) {
        throw std::runtime_error("Cannot open hex output file: " + filename);
    }

    for (const auto& sec : mergedSections) {
        auto it = sectionBases.find(sec.name);

        if (it == sectionBases.end()) {
            throw std::runtime_error("No base address for section: " + sec.name);
        }

        uint32_t base = it->second;

        for (size_t i = 0; i < sec.data.size(); i += 8) {
            out << std::hex << std::uppercase
                << std::setw(8) << std::setfill('0')
                << (base + i)
                << ": ";

            size_t end = std::min(i + 8, sec.data.size());

            for (size_t j = i; j < end; j++) {
                out << std::setw(2) << std::setfill('0')
                    << (int)sec.data[j];

                if (j + 1 < end) {
                    out << " ";
                }
            }

            out << "\n";
        }
    }

    out << std::dec;
}

void Linker::checkSectionOverlaps()
{
    for (int i = 0; i < (int)mergedSections.size(); i++) {
        const auto& s1 = mergedSections[i];

        uint32_t start1 = sectionBases[s1.name];
        uint32_t end1 = start1 + s1.data.size();

        for (int j = i + 1; j < (int)mergedSections.size(); j++) {
            const auto& s2 = mergedSections[j];

            uint32_t start2 = sectionBases[s2.name];
            uint32_t end2 = start2 + s2.data.size();

            bool overlap = start1 < end2 && start2 < end1;

            if (overlap) {
                throw std::runtime_error(
                    "Section overlap: " + s1.name + " and " + s2.name
                );
            }
        }
    }
}

void Linker::checkUndefinedSymbols()
{
    for (const auto& rel : globalRelocations)
    {
        findResolvedSymbolAddress(rel.symbol);
    }
}

void Linker::writeRelocatableObject(const std::string& filename)
{
    std::ofstream out(filename);
    if (!out) {
        throw std::runtime_error("Cannot open relocatable output file: " + filename);
    }

    out << "#SS_OBJECT\n\n";

    out << "[SECTIONS]\n";
    out << mergedSections.size() << "\n";

    for (const auto& sec : mergedSections) {
        out << sec.name << " " << sec.data.size() << "\n";

        for (uint8_t byte : sec.data) {
            out << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << (int)byte << " ";
        }

        out << std::dec << "\n";
    }

    out << "\n[SYMBOLS]\n";
    out << globalSymbols.size() << "\n";

    for (int i = 0; i < (int)globalSymbols.size(); i++) {
        const GlobalSymbol& s = globalSymbols[i];

        out << i << " "
            << s.name << " "
            << s.section << " "
            << s.value << " "
            << (s.global ? "GLOBAL" : "LOCAL") << " "
            << "DEFINED"
            << "\n";
    }

    out << "\n[RELOCATIONS]\n";
    out << globalRelocations.size() << "\n";

    for (const auto& r : globalRelocations) {
        out << r.section << " "
            << r.offset << " "
            << r.symbol << " "
            << (r.type == LinkRelocationType::ABS32 ? "ABS32" : "DISP12")
            << "\n";
    }

    out << "\n[END]\n";
}