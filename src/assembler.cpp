#include "../inc/assembler.hpp"

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <iomanip>

//Private functions
void Assembler::checkDisp12(int32_t value)
{
    if (value < -2048 || value > 2047) {
        throw std::runtime_error("Displacement does not fit in signed 12 bits");
    }
}

void Assembler::emitInstruction(uint8_t oc, uint8_t mod, uint8_t a, uint8_t b, uint8_t c, int16_t disp)
{
    uint32_t d = disp & 0x0FFF;

    uint32_t instr =
        ((uint32_t)oc  << 28) |
        ((uint32_t)mod << 24) |
        ((uint32_t)a   << 20) |
        ((uint32_t)b   << 16) |
        ((uint32_t)c   << 12) |
        d;

    emit32(instr);
}

void Assembler::emit32(uint32_t value)

{

    if (currentSection == -1) {

        throw std::runtime_error("instruction outside of section");

    }

    Section& sec = sections[currentSection];

    sec.data.push_back(value & 0xFF);

    sec.data.push_back((value >> 8) & 0xFF);

    sec.data.push_back((value >> 16) & 0xFF);

    sec.data.push_back((value >> 24) & 0xFF);

}

int Assembler::getOrCreateSymbol(const std::string& name)
{
    auto it = symbolMap.find(name);
    if (it != symbolMap.end()) {
        return it->second;
    }

    int id = symbols.size();
    symbols.emplace_back(name);
    symbolMap[name] = id;
    return id;
}

//Public functions
void Assembler::startSection(const std::string& name)
{
    sections.emplace_back(name);
    currentSection = sections.size() - 1;
}

void Assembler::defineLabel(const std::string& name)
{
    if (currentSection == -1) {
        throw std::runtime_error("Label defined outside of section: " + name);
    }

    int id = getOrCreateSymbol(name);
    Symbol& sym = symbols[id];

    if (sym.defined) {
        throw std::runtime_error("Symbol already defined: " + name);
    }

    sym.sectionId = currentSection;
    sym.value = sections[currentSection].data.size();
    sym.defined = true;
}

void Assembler::addGlobal(const std::string& name)
{
    int id = getOrCreateSymbol(name);
    symbols[id].global = true;
}

void Assembler::addExtern(const std::string& name)
{
    int id = getOrCreateSymbol(name);
    symbols[id].external = true;
    symbols[id].global = true;
    symbols[id].defined = false;
    symbols[id].sectionId = -1;
}

void Assembler::emitWord(uint32_t value)
{
    if (currentSection == -1) {
        throw std::runtime_error(".word outside of section");
    }

    Section& sec = sections[currentSection];

    sec.data.push_back(value & 0xFF);
    sec.data.push_back((value >> 8) & 0xFF);
    sec.data.push_back((value >> 16) & 0xFF);
    sec.data.push_back((value >> 24) & 0xFF);
}
void Assembler::emitWordSymbol(const std::string& symbolName)

{

    if (currentSection == -1) {

        throw std::runtime_error(".word outside of section");

    }

    int id=getOrCreateSymbol(symbolName);
    Symbol& sym= symbols[id];
    if(sym.defined && sym.absolute){
        emitWord(sym.value);
        return;
    }

    Section& sec = sections[currentSection];

    uint32_t offset = sec.data.size();

    sec.data.push_back(0);

    sec.data.push_back(0);

    sec.data.push_back(0);

    sec.data.push_back(0);

    relocations.emplace_back(currentSection, offset, symbolName, RelocationType::ABS32);

}
void Assembler::emitSkip(uint32_t size)
{
    if (currentSection == -1) {
        throw std::runtime_error(".skip outside of section");
    }

    Section& sec = sections[currentSection];

    for (uint32_t i = 0; i < size; i++) {
        sec.data.push_back(0);
    }
}

void Assembler::emitHalt()
{
    emitInstruction(0, 0, 0, 0, 0, 0);
}

void Assembler::emitInt()
{
    emitInstruction(1, 0, 0, 0, 0, 0);
}

void Assembler::emitXchg(int gprS, int gprD)
{
    emitInstruction(0x4, 0x0, 0, gprD, gprS, 0);
}

void Assembler::emitAdd(int gprS, int gprD)
{
    emitInstruction(0x5, 0x0, gprD, gprD, gprS, 0);
}

void Assembler::emitSub(int gprS, int gprD)
{
    emitInstruction(0x5, 0x1, gprD, gprD, gprS, 0);
}

void Assembler::emitMul(int gprS, int gprD)
{
    emitInstruction(0x5, 0x2, gprD, gprD, gprS, 0);
}

void Assembler::emitDiv(int gprS, int gprD)
{
    emitInstruction(0x5, 0x3, gprD, gprD, gprS, 0);
}

void Assembler::emitNot(int gpr)
{
    emitInstruction(0x6, 0x0, gpr, gpr, 0, 0);
}

void Assembler::emitAnd(int gprS, int gprD)
{
    emitInstruction(0x6, 0x1, gprD, gprD, gprS, 0);
}

void Assembler::emitOr(int gprS, int gprD)
{
    emitInstruction(0x6, 0x2, gprD, gprD, gprS, 0);
}

void Assembler::emitXor(int gprS, int gprD)
{
    emitInstruction(0x6, 0x3, gprD, gprD, gprS, 0);
}

void Assembler::emitShl(int gprS, int gprD)
{
    emitInstruction(0x7, 0x0, gprD, gprD, gprS, 0);
}

void Assembler::emitShr(int gprS, int gprD)
{
    emitInstruction(0x7, 0x1, gprD, gprD, gprS, 0);
}

void Assembler::emitPush(int gpr)
{
    // sp <= sp - 4; mem32[sp] <= gpr;
    emitInstruction(0x8, 0x1, 14, 0, gpr, -4);
}

void Assembler::emitPop(int gpr)
{
    // gpr <= mem32[sp]; sp <= sp + 4;
    emitInstruction(0x9, 0x3, gpr, 14, 0, 4);
}

void Assembler::emitRet()
{
    // pop pc
    emitPop(15);
}

void Assembler::emitIret()
{
    // pop pc; pop status;
    emitInstruction(0x9, 0x3, 15, 14, 0, 4);  // pop pc
    emitInstruction(0x9, 0x7, 0, 14, 0, 4);   // pop status
}

void Assembler::emitCsrrd(int csr, int gpr)
{
    // gpr <= csr
    emitInstruction(0x9, 0x0, gpr, csr, 0, 0);
}

void Assembler::emitCsrwr(int gpr, int csr)
{
    // csr <= gpr
    emitInstruction(0x9, 0x4, csr, gpr, 0, 0);
}


void Assembler::emitCallLiteral(int32_t value)
{
    checkDisp12(value);
    emitInstruction(0x2, 0x0, 0, 0, 0, value);
}

void Assembler::emitJmpLiteral(int32_t value)
{
    checkDisp12(value);
    emitInstruction(0x3, 0x0, 0, 0, 0, value);
}

void Assembler::emitBeqLiteral(int r1, int r2, int32_t value)
{
    checkDisp12(value);
    emitInstruction(0x3, 0x1, 0, r1, r2, value);
}

void Assembler::emitBneLiteral(int r1, int r2, int32_t value)
{
    checkDisp12(value);
    emitInstruction(0x3, 0x2, 0, r1, r2, value);
}

void Assembler::emitBgtLiteral(int r1, int r2, int32_t value)
{
    checkDisp12(value);
    emitInstruction(0x3, 0x3, 0, r1, r2, value);
}

void Assembler::emitCallSymbol(const std::string& symbolName)
{
    getOrCreateSymbol(symbolName);
    uint32_t offset = sections[currentSection].data.size();
    emitInstruction(0x2, 0x0, 0, 0, 0, 0);
    relocations.emplace_back(currentSection, offset, symbolName, RelocationType::DISP12);
}

void Assembler::emitJmpSymbol(const std::string& symbolName)
{
    getOrCreateSymbol(symbolName);
    uint32_t offset = sections[currentSection].data.size();
    emitInstruction(0x3, 0x0, 0, 0, 0, 0);
    relocations.emplace_back(currentSection, offset, symbolName, RelocationType::DISP12);
}

void Assembler::emitBeqSymbol(int r1, int r2, const std::string& symbolName)
{
    getOrCreateSymbol(symbolName);
    uint32_t offset = sections[currentSection].data.size();
    emitInstruction(0x3, 0x1, 0, r1, r2, 0);
    relocations.emplace_back(currentSection, offset, symbolName, RelocationType::DISP12);
}

void Assembler::emitBneSymbol(int r1, int r2, const std::string& symbolName)
{
    getOrCreateSymbol(symbolName);
    uint32_t offset = sections[currentSection].data.size();
    emitInstruction(0x3, 0x2, 0, r1, r2, 0);
    relocations.emplace_back(currentSection, offset, symbolName, RelocationType::DISP12);
}

void Assembler::emitBgtSymbol(int r1, int r2, const std::string& symbolName)
{
    getOrCreateSymbol(symbolName);
    uint32_t offset = sections[currentSection].data.size();
    emitInstruction(0x3, 0x3, 0, r1, r2, 0);
    relocations.emplace_back(currentSection, offset, symbolName, RelocationType::DISP12);
}


void Assembler::emitLdImmediateLiteral(int32_t value, int gprD)
{
    checkDisp12(value);
    // gpr[A] <= gpr[B] + D
    // gprD <= r0 + value
    emitInstruction(0x9, 0x1, gprD, 0, 0, value);
}

void Assembler::emitLdRegister(int gprS, int gprD)
{
    // gprD <= gprS + 0
    emitInstruction(0x9, 0x1, gprD, gprS, 0, 0);
}

void Assembler::emitLdMemReg(int gprAddr, int gprD)
{
    // gprD <= mem32[gprAddr + r0 + 0]
    emitInstruction(0x9, 0x2, gprD, gprAddr, 0, 0);
}

void Assembler::emitLdMemRegLiteral(int gprAddr, int32_t disp, int gprD)
{
    checkDisp12(disp);
    // gprD <= mem32[gprAddr + r0 + disp]
    emitInstruction(0x9, 0x2, gprD, gprAddr, 0, disp);
}

void Assembler::emitLdImmediateSymbol(const std::string& symbolName, int gprD)
{
    int id = getOrCreateSymbol(symbolName);
    Symbol& sym = symbols[id];

    if (sym.defined && sym.absolute) {
        emitLdImmediateLiteral((int32_t)sym.value, gprD);
        return;
    }

    // ld [%pc + 4], gprD
    emitInstruction(0x9, 0x2, gprD, 15, 0, 4);

    // jmp 8
    emitInstruction(0x3, 0x0, 15, 0, 0, 8);

    // literal pool entry
    emitWordSymbol(symbolName);
}

void Assembler::emitLdMemSymbol(const std::string& symbolName, int gprD)
{
    int id = getOrCreateSymbol(symbolName);
    Symbol& sym = symbols[id];

    if (sym.defined && sym.absolute) {
        // load value from absolute address
        emitInstruction(0x9, 0x2, gprD, 0, 0, (int32_t)sym.value);
        return;
    }

    // first load address of symbol into gprD
    emitLdImmediateSymbol(symbolName, gprD);

    // then load from memory at that address
    emitLdMemReg(gprD, gprD);
}

void Assembler::emitLdMemRegSymbol(int gprAddr, const std::string& symbolName, int gprD)

{
    int id = getOrCreateSymbol(symbolName);
    Symbol& sym = symbols[id];
    if (sym.defined && sym.absolute) {
        emitLdMemRegLiteral(gprAddr, (int32_t)sym.value, gprD);
        return;
    }

    uint32_t offset = sections[currentSection].data.size();

    // gprD <= mem32[gprAddr + r0 + symbol]

    emitInstruction(0x9, 0x2, gprD, gprAddr, 0, 0);

    relocations.emplace_back(currentSection, offset, symbolName, RelocationType::DISP12);

}

void Assembler::emitStMemSymbol(int gprS, const std::string& symbolName)
{
    int id = getOrCreateSymbol(symbolName);
    Symbol& sym = symbols[id];

    if (sym.defined && sym.absolute) {
        emitInstruction(0x8, 0x0, 0, 0, gprS, (int32_t)sym.value);
        return;
    }

    // use temporary register r13 for address
    emitLdImmediateSymbol(symbolName, 13);

    // store gprS to memory[r13]
    emitStMemReg(gprS, 13);
}

void Assembler::emitStMemReg(int gprS, int gprAddr)
{
    // mem32[gprAddr + r0 + 0] <= gprS
    emitInstruction(0x8, 0x0, gprAddr, 0, gprS, 0);
}

void Assembler::emitStMemRegLiteral(int gprS, int gprAddr, int32_t disp)
{
    checkDisp12(disp);
    // mem32[gprAddr + r0 + disp] <= gprS
    emitInstruction(0x8, 0x0, gprAddr, 0, gprS, disp);
}

void Assembler::emitStMemRegSymbol(int gprS, int gprAddr, const std::string& symbolName)
{
    int id = getOrCreateSymbol(symbolName);
    Symbol& sym = symbols[id];
    if (sym.defined && sym.absolute) {
        emitStMemRegLiteral(gprS, gprAddr, (int32_t)sym.value);
        return;
    }

    uint32_t offset = sections[currentSection].data.size();

    // mem32[gprAddr + r0 + symbol] <= gprS

    emitInstruction(0x8, 0x0, gprAddr, 0, gprS, 0);

    relocations.emplace_back(currentSection, offset, symbolName, RelocationType::DISP12);
}

void Assembler::writeObjectFile(const std::string& filename)
{
    std::ofstream out(filename);
    if (!out) {
        throw std::runtime_error("Cannot open output file: " + filename);
    }

    out << "#SS_OBJECT\n\n";

    out << "[SECTIONS]\n";
    out << sections.size() << "\n";

    for (int i = 0; i < (int)sections.size(); i++) {
        const Section& sec = sections[i];

        out << sec.name << " " << sec.data.size() << "\n";

        for (uint8_t byte : sec.data) {
            out << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << (int)byte << " ";
        }

        out << std::dec << "\n";
    }

    out << "\n[SYMBOLS]\n";
    out << symbols.size() << "\n";

    for (int i = 0; i < (int)symbols.size(); i++) {
        const Symbol& s = symbols[i];

        std::string sectionName = "UND";
        if (s.absolute) {
            sectionName = "ABS";
        }
        else if (s.sectionId >= 0) {
            sectionName = sections[s.sectionId].name;
        }

        out << i << " "
            << s.name << " "
            << sectionName << " "
            << s.value << " "
            << (s.global ? "GLOBAL" : "LOCAL") << " "
            << (s.external ? "EXTERN" : (s.defined ? "DEFINED" : "UNDEFINED"))
            << "\n";
    }

    out << "\n[RELOCATIONS]\n";
    out << relocations.size() << "\n";

    for (const Relocation& r : relocations) {
        out << sections[r.sectionId].name << " "
            << r.offset << " "
            << r.symbol << " "
            << (r.type == RelocationType::ABS32 ? "ABS32" : "DISP12")
            << "\n";
    }

    out << "\n[END]\n";
}

void Assembler::printState()
{
    std::cout << "\n=== Sections ===\n";
    for (int i = 0; i < (int)sections.size(); i++) {
        std::cout << i << ": " << sections[i].name
                  << " size=" << sections[i].data.size()
                  << std::endl;
    }

    std::cout << "\n=== Symbols ===\n";
    for (int i = 0; i < (int)symbols.size(); i++) {
        const Symbol& s = symbols[i];

        std::cout << i
                  << ": " << s.name
                  << " section=" << s.sectionId
                  << " value=" << s.value
                  << " global=" << s.global
                  << " external=" << s.external
                  << " defined=" << s.defined
                  << std::endl;
    }

    std::cout << "\n=== Relocations ===\n";
    for (int i = 0; i < (int)relocations.size(); i++) {

    const Relocation& r = relocations[i];

    std::cout << i

              << ": section=" << r.sectionId

              << " offset=" << r.offset

              << " symbol=" << r.symbol

              << " type=" << (r.type == RelocationType::ABS32 ? "ABS32" : "DISP12")

              << std::endl;

}
}

void Assembler::emitAscii(const std::string& text)
{
    if (currentSection == -1) {
        throw std::runtime_error(".ascii outside of section");
    }

    Section& sec = sections[currentSection];

    for (char c : text) {
        sec.data.push_back((uint8_t)c);
    }
}

void Assembler::defineEqu(const std::string& name, uint32_t value)
{
    int id = getOrCreateSymbol(name);
    Symbol& sym = symbols[id];

    if (sym.defined) {
        throw std::runtime_error("Symbol already defined: " + name);
    }

    sym.sectionId = -1;
    sym.value = value;
    sym.global = false;
    sym.defined = true;
    sym.external = false;
    sym.absolute = true;
}

uint32_t Assembler::getAbsoluteSymbolValue(const std::string& name)
{
    int id = getOrCreateSymbol(name);
    Symbol& sym = symbols[id];

    if (!sym.defined || !sym.absolute) {
        throw std::runtime_error("Invalid symbol in .equ expression: " + name);
    }

    return sym.value;
}