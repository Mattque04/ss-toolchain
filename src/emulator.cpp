#include "../inc/emulator.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

Emulator::~Emulator()
{
    restoreTerminal();
}

void Emulator::configureTerminal()
{
    if (!isatty(STDIN_FILENO)) {
        return;
    }

    if (tcgetattr(STDIN_FILENO, &originalTerminalSettings) == -1) {
        throw std::runtime_error("Cannot read terminal settings");
    }

    termios settings = originalTerminalSettings;
    settings.c_lflag &= ~(ICANON | ECHO);
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == -1) {
        throw std::runtime_error("Cannot configure terminal");
    }

    terminalConfigured = true;
}

void Emulator::restoreTerminal()
{
    if (!terminalConfigured) {
        return;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &originalTerminalSettings);
    terminalConfigured = false;
}

void Emulator::checkTerminal()
{
    pollfd input{};
    input.fd = STDIN_FILENO;
    input.events = POLLIN;

    int result = poll(&input, 1, 0);
    if (result <= 0 || !(input.revents & POLLIN)) {
        return;
    }

    char character;
    if (read(STDIN_FILENO, &character, 1) == 1) {
        terminalCharacter = static_cast<uint8_t>(character);
        terminalInterruptPending = true;
    }
}

uint32_t Emulator::getTimerPeriod() const
{
    switch (timerConfig) {
    case 0: return 500;
    case 1: return 1000;
    case 2: return 1500;
    case 3: return 2000;
    case 4: return 5000;
    case 5: return 10000;
    case 6: return 30000;
    case 7: return 60000;
    default: return 500;
    }
}

void Emulator::resetTimer()
{
    nextTimerInterrupt = std::chrono::steady_clock::now() + std::chrono::milliseconds(getTimerPeriod());
}

void Emulator::checkTimer()
{
    if (std::chrono::steady_clock::now() < nextTimerInterrupt) {
        return;
    }

    timerInterruptPending = true;
    resetTimer();
}

uint8_t Emulator::read8(uint32_t address) const
{
    auto it = memory.find(address);

    if (it == memory.end()) {
        return 0;
    }

    return it->second;
}

void Emulator::write8(uint32_t address, uint8_t value)
{
    memory[address] = value;
}

void Emulator::loadHexFile(const std::string& filename)
{
    std::ifstream in(filename);

    if (!in) {
        throw std::runtime_error("Cannot open hex file: " + filename);
    }

    std::string line;

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        size_t colon = line.find(':');

        if (colon == std::string::npos) {
            throw std::runtime_error("Invalid hex line: " + line);
        }

        std::string addressText = line.substr(0, colon);
        size_t parsedAddressCharacters = 0;
        uint64_t address = std::stoull(
            addressText,
            &parsedAddressCharacters,
            16
        );

        if (parsedAddressCharacters != addressText.size() ||
            address > std::numeric_limits<uint32_t>::max())
        {
            throw std::runtime_error("Invalid hex address: " + addressText);
        }

        std::string bytesText = line.substr(colon + 1);
        std::stringstream ss(bytesText);

        std::string byteText;
        uint64_t currentAddress = address;
        bool hasBytes = false;

        while (ss >> byteText) {
            size_t parsedByteCharacters = 0;
            uint64_t byteValue = std::stoull(
                byteText,
                &parsedByteCharacters,
                16
            );

            if (parsedByteCharacters != byteText.size() ||
                byteValue > 0xFF ||
                currentAddress > std::numeric_limits<uint32_t>::max())
            {
                throw std::runtime_error("Invalid hex byte: " + byteText);
            }

            write8(
                static_cast<uint32_t>(currentAddress),
                static_cast<uint8_t>(byteValue)
            );
            currentAddress++;
            hasBytes = true;
        }

        if (!hasBytes) {
            throw std::runtime_error("Hex line has no data: " + line);
        }
    }
}

void Emulator::printMemory() const
{
    std::cout << "Loaded memory bytes: " << memory.size() << "\n";

    for (const auto& [address, value] : memory) {
        std::cout
            << "0x"
            << std::hex
            << std::setw(8)
            << std::setfill('0')
            << address
            << ": "
            << std::setw(2)
            << (int)value
            << std::dec
            << "\n";
    }
}

uint32_t Emulator::read32(uint32_t address)
{
    if (address == 0xFFFFFF04) {
        uint8_t ch = terminalCharacter;

        terminalCharacter = 0;
        terminalInterruptPending = false;

        return static_cast<uint32_t>(ch);
    }

    return
        ((uint32_t)read8(address + 0)) |
        ((uint32_t)read8(address + 1) << 8) |
        ((uint32_t)read8(address + 2) << 16) |
        ((uint32_t)read8(address + 3) << 24);
}

void Emulator::write32(uint32_t address, uint32_t value)
{
    if (address == 0xFFFFFF00) {

        std::cout << (char)(value & 0xFF);
        std::cout.flush();
        return;
    }

    if (address == 0xFFFFFF10) {
        timerConfig = value;
        resetTimer();
        return;
    }

    write8(address + 0, value & 0xFF);
    write8(address + 1, (value >> 8) & 0xFF);
    write8(address + 2, (value >> 16) & 0xFF);
    write8(address + 3, (value >> 24) & 0xFF);
}

void Emulator::run()
{
    gpr[15] = 0x40000000; // pc entry
    gpr[14] = 0x10000000; // sp entry

    configureTerminal();
    resetTimer();

    while (true) {
        uint32_t pc = gpr[15];
        uint32_t instr = read32(pc);

        gpr[15] += 4;

        uint8_t oc  = (instr >> 28) & 0xF;
        uint8_t mod = (instr >> 24) & 0xF;
        uint8_t a   = (instr >> 20) & 0xF;
        uint8_t b   = (instr >> 16) & 0xF;
        uint8_t c   = (instr >> 12) & 0xF;
        int32_t d   = instr & 0xFFF;
        if (d & 0x800) {
            d |= 0xFFFFF000;
        }

    switch (oc) {
        case 0x0: // halt
            restoreTerminal();
            std::cout << "-----------------------------------------------------------------\n";
            std::cout << "Emulated processor executed halt instruction\n";
            printRegisters();
            return;

        case 0x4: { // xchg
            uint32_t tmp = gpr[b];
            gpr[b] = gpr[c];
            gpr[c] = tmp;
            break;
        }

        case 0x5: // add/sub/mul/div
            if (mod == 0) gpr[a] = gpr[b] + gpr[c];
            else if (mod == 1) gpr[a] = gpr[b] - gpr[c];
            else if (mod == 2) gpr[a] = gpr[b] * gpr[c];
            else if (mod == 3 && gpr[c] != 0) gpr[a] = gpr[b] / gpr[c];
            else enterInterrupt(1);
            break;

        case 0x6: // not/and/or/xor
            if (mod == 0) gpr[a] = ~gpr[b];
            else if (mod == 1) gpr[a] = gpr[b] & gpr[c];
            else if (mod == 2) gpr[a] = gpr[b] | gpr[c];
            else if (mod == 3) gpr[a] = gpr[b] ^ gpr[c];
            else enterInterrupt(1);
            break;

        case 0x7: // shl/shr
            if (gpr[c] >= 32) {
                enterInterrupt(1);
            }
            else if (mod == 0) gpr[a] = gpr[b] << gpr[c];
            else if (mod == 1) gpr[a] = gpr[b] >> gpr[c];
            else enterInterrupt(1);
            break;

        case 0x8: // store
            if (mod == 0x0) {
                write32(gpr[a] + gpr[b] + d, gpr[c]);
            }
            else if (mod == 0x1) {
                gpr[a] = gpr[a] + d;
                write32(gpr[a], gpr[c]);
            }
            else if (mod == 0x2) {
                uint32_t addr = read32(gpr[a] + gpr[b] + d);
                write32(addr, gpr[c]);
            }
            else {
                enterInterrupt(1);
            }
            break;
        case 0x9: // load / csr
            if (mod == 0x0) {
                if (b < 3) gpr[a] = csr[b];
                else enterInterrupt(1);
            }
            else if (mod == 0x1) {
                    gpr[a] = gpr[b] + d;
            }
            else if (mod == 0x2) {
                gpr[a] = read32(gpr[b] + gpr[c] + d);
            }
            else if (mod == 0x3) {
                gpr[a] = read32(gpr[b]);
                gpr[b] = gpr[b] + d;
            }
            else if (mod == 0x4) {
                if (a < 3) csr[a] = gpr[b];
                else enterInterrupt(1);
            }
            else if (mod == 0x5) {
                if (a < 3 && b < 3) csr[a] = csr[b] | d;
                else enterInterrupt(1);
            }
            else if (mod == 0x6) {
                if (a < 3) csr[a] = read32(gpr[b] + gpr[c] + d);
                else enterInterrupt(1);
            }
            else if (mod == 0x7) {
                if (a < 3) {
                    csr[a] = read32(gpr[b]);
                    gpr[b] = gpr[b] + d;
                }
                else {
                    enterInterrupt(1);
                }
            }
            else if (mod == 0x8){
                gpr[15] = read32(gpr[14]);
                gpr[14] += 4;

                csr[0] = read32(gpr[14]);
                gpr[14] += 4;
            }
            else {
                enterInterrupt(1);
            }
            break;
        case 0x2: // call
            if (mod == 0x0) {
                gpr[14] -= 4;
                write32(gpr[14], gpr[15]);

                gpr[15] = gpr[a] + d;
            }
            else if (mod == 0x1) {
                gpr[14] -= 4;
                write32(gpr[14], gpr[15]);

                gpr[15] = read32(gpr[a] + gpr[b] + d);
            }
            else {
                enterInterrupt(1);
            }
            break;

        case 0x3: // jumps
            if (mod == 0x0) {
                gpr[15] = gpr[a] + d;
            }
            else if (mod == 0x1) {
                if (gpr[b] == gpr[c]) {
                    gpr[15] = gpr[a] + d;
                }
            }
            else if (mod == 0x2) {
                if (gpr[b] != gpr[c]) {
                    gpr[15] = gpr[a] + d;
                }
            }
            else if (mod == 0x3) {
                if ((int32_t)gpr[b] > (int32_t)gpr[c]) {
                    gpr[15] = gpr[a] + d;
                }
            }
            else if (mod == 0x8) {
                gpr[15] = read32(gpr[a] + d);
            }
            else if (mod == 0x9) {
                if (gpr[b] == gpr[c]) {
                    gpr[15] = read32(gpr[a] + d);
                }
            }
            else if (mod == 0xA) {
                if (gpr[b] != gpr[c]) {
                    gpr[15] = read32(gpr[a] + d);
                }
            }
            else if (mod == 0xB) {
                if ((int32_t)gpr[b] > (int32_t)gpr[c]) {
                    gpr[15] = read32(gpr[a] + d);
                }
            }
            else {
                enterInterrupt(1);
            }
            break;
        case 0x1: // INT
            enterInterrupt(4);
            break;
        default:
            enterInterrupt(1);
            break;
        }


        gpr[0] = 0;
        checkTerminal();
        checkTimer();

        if (terminalInterruptPending) {
            bool globalMasked = csr[0] & (1 << 2);
            bool terminalMasked = csr[0] & (1 << 1);
            bool handlerSet = csr[1] != 0;

            if (handlerSet && !globalMasked && !terminalMasked) {
                terminalInterruptPending = false;
                enterInterrupt(3);
            }
        }

        if (timerInterruptPending) {
            bool globalMasked = csr[0] & (1 << 2);
            bool timerMasked = csr[0] & 1;
            bool handlerSet = csr[1] != 0;

            if (handlerSet && !globalMasked && !timerMasked) {
                timerInterruptPending = false;
                enterInterrupt(2);
            }
        }
                
    }
}

void Emulator::printRegisters() const
{
    std::cout << "Emulated processor state:\n";

    for (int i = 0; i < 16; i++) {
        if (i % 4 == 0 && i < 10) {
            std::cout << " ";
        }

        std::cout << "r" << std::dec << i << "=0x"
                  << std::hex << std::setw(8) << std::setfill('0')
                  << gpr[i];

        if (i % 4 == 3) {
            std::cout << "\n";
        } else {
            std::cout << "    ";
        }
    }

    std::cout << std::dec;
}

void Emulator::push32(uint32_t value)
{
    gpr[14] -= 4;
    write32(gpr[14], value);
}

uint32_t Emulator::pop32()
{
    uint32_t value = read32(gpr[14]);
    gpr[14] += 4;
    return value;
}

void Emulator::enterInterrupt(uint32_t causeValue)
{
    csr[2] = causeValue;

    push32(csr[0]);
    push32(gpr[15]);

    csr[0] |= 0x4;
    gpr[15] = csr[1];
}
