#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "symbol.hpp"
#include "section.hpp"
#include "relocation.hpp"

class Assembler {
public:
    // .section text
    void startSection(const std::string& name);

    // main:
    void defineLabel(const std::string& name);

    // .global main
    void addGlobal(const std::string& name);

    // .extern printf
    void addExtern(const std::string& name);

    void defineEqu(const std::string& name, uint32_t value);
    
    uint32_t getAbsoluteSymbolValue(const std::string& name);

    // .word 5
    void emitWord(uint32_t value);

    // .word main
    void emitWordSymbol(const std::string& symbolName);

    // .skip 16
    void emitSkip(uint32_t size);

    // halt
    void emitHalt();

    // int
    void emitInt();

    // xchg %r1, %r2
    void emitXchg(int gprS, int gprD);

    // add %r1, %r2
    void emitAdd(int gprS, int gprD);

    // sub %r1, %r2
    void emitSub(int gprS, int gprD);

    // mul %r1, %r2
    void emitMul(int gprS, int gprD);

    // div %r1, %r2
    void emitDiv(int gprS, int gprD);

    // not %r1
    void emitNot(int gpr);

    // and %r1, %r2
    void emitAnd(int gprS, int gprD);

    // or %r1, %r2
    void emitOr(int gprS, int gprD);

    // xor %r1, %r2
    void emitXor(int gprS, int gprD);

    // shl %r1, %r2
    void emitShl(int gprS, int gprD);

    // shr %r1, %r2
    void emitShr(int gprS, int gprD);

    // push %r1
    void emitPush(int gpr);

    // pop %r1
    void emitPop(int gpr);

    // ret
    void emitRet();

    // iret
    void emitIret();

    // csrrd %status, %r1
    void emitCsrrd(int csr, int gpr);

    // csrwr %r1, %status
    void emitCsrwr(int gpr, int csr);

    // call 100
    void emitCallLiteral(int32_t value);

    // jmp 100
    void emitJmpLiteral(int32_t value);

    // beq %r1, %r2, 100
    void emitBeqLiteral(int r1, int r2, int32_t value);

    // bne %r1, %r2, 100
    void emitBneLiteral(int r1, int r2, int32_t value);

    // bgt %r1, %r2, 100
    void emitBgtLiteral(int r1, int r2, int32_t value);

    // call func
    void emitCallSymbol(const std::string& symbolName);

    // jmp loop
    void emitJmpSymbol(const std::string& symbolName);

    // beq %r1, %r2, loop
    void emitBeqSymbol(int r1, int r2, const std::string& symbolName);

    // bne %r1, %r2, loop
    void emitBneSymbol(int r1, int r2, const std::string& symbolName);

    // bgt %r1, %r2, loop
    void emitBgtSymbol(int r1, int r2, const std::string& symbolName);

    // ld $5, %r1
    void emitLdImmediateLiteral(int32_t value, int gprD);

    // ld %r2, %r1
    void emitLdRegister(int gprS, int gprD);

    // ld [%r2], %r1
    void emitLdMemReg(int gprAddr, int gprD);

    // ld [%r2 + 8], %r1
    void emitLdMemRegLiteral(int gprAddr, int32_t disp, int gprD);

    // ld $main, %r1
    void emitLdImmediateSymbol(const std::string& symbolName, int gprD);

    // ld main, %r1
    void emitLdMemSymbol(const std::string& symbolName, int gprD);

    // ld [%r2 + main], %r1
    void emitLdMemRegSymbol(int gprAddr, const std::string& symbolName, int gprD);

    // st %r1, main
    void emitStMemSymbol(int gprS, const std::string& symbolName);

    // st %r1, [%r2]
    void emitStMemReg(int gprS, int gprAddr);

    // st %r1, [%r2 + 8]
    void emitStMemRegLiteral(int gprS, int gprAddr, int32_t disp);

    // st %r1, [%r2 + main]
    void emitStMemRegSymbol(int gprS, int gprAddr, const std::string& symbolName);

    // upis predmetnog programa u .o
    void writeObjectFile(const std::string& filename);

    // debug ispis internog stanja
    void printState();

    void emitAscii(const std::string& text);

private:
    // provera da li displacement staje u 12 bita
    void checkDisp12(int32_t value);

    // emitovanje jedne masinske instrukcije
    void emitInstruction(uint8_t oc, uint8_t mod, uint8_t a, uint8_t b, uint8_t c, int16_t disp);

    // emitovanje 4 bajta u trenutnu sekciju
    void emit32(uint32_t value);

    // vraca postojeci simbol ili pravi novi
    int getOrCreateSymbol(const std::string& name);

    std::vector<Section> sections;
    std::vector<Symbol> symbols;
    std::vector<Relocation> relocations;

    std::unordered_map<std::string, int> symbolMap;

    int currentSection = -1;
};