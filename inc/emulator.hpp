#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <chrono>

class Emulator {
public:
    void loadHexFile(const std::string& filename);
    void printMemory() const;
    void run();

private:
    //terminal things
    std::atomic<bool> terminalInterruptPending = false;
    std::atomic<uint8_t> terminalCharacter = 0;
    std::thread terminalThread;
    void terminalWorker();

    //timer things
    std::atomic<bool> timerInterruptPending = false;
    std::atomic<uint32_t> timerConfig = 0;
    std::thread timerThread;
    void timerWorker();

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