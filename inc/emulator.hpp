#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <chrono>
#include <termios.h>

class Emulator {
public:
    ~Emulator();

    void loadHexFile(const std::string& filename);
    void printMemory() const;
    void run();

private:
    bool terminalInterruptPending = false;
    uint8_t terminalCharacter = 0;
    bool terminalConfigured = false;
    termios originalTerminalSettings{};

    void configureTerminal();
    void restoreTerminal();
    void checkTerminal();

    bool timerInterruptPending = false;
    uint32_t timerConfig = 0;
    std::chrono::steady_clock::time_point nextTimerInterrupt;

    uint32_t getTimerPeriod() const;
    void resetTimer();
    void checkTimer();

    void push32(uint32_t value);
    uint32_t pop32();

    void enterInterrupt(uint32_t causeValue);

    std::unordered_map<uint32_t, uint8_t> memory;

    uint8_t read8(uint32_t address) const;
    void write8(uint32_t address, uint8_t value);

    uint32_t gpr[16] = {};
    uint32_t csr[3] = {}; // 0=status, 1=handler, 2=cause

    uint32_t read32(uint32_t address);
    void write32(uint32_t address, uint32_t value);

    void printRegisters() const;
};
