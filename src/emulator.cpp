#include "../inc/emulator.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

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
        uint32_t address = std::stoul(addressText, nullptr, 16);

        std::string bytesText = line.substr(colon + 1);
        std::stringstream ss(bytesText);

        std::string byteText;
        uint32_t currentAddress = address;

        while (ss >> byteText) {
            uint8_t byte =
                static_cast<uint8_t>(std::stoul(byteText, nullptr, 16));

            write8(currentAddress, byte);
            currentAddress++;
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
    uint8_t ch = terminalCharacter.load();

    terminalCharacter.store(0);
    terminalInterruptPending.store(false);

    return (uint32_t)ch;

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
      //  std::cerr << "term_out value=0x" << std::hex << value << std::dec << "\n";

        std::cout << (char)(value & 0xFF);
        std::cout.flush();
        return;
    }

    if (address == 0xFFFFFF10) {
        timerConfig.store(value);
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

    terminalThread = std::thread(&Emulator::terminalWorker, this);
    terminalThread.detach();

    timerThread = std::thread(&Emulator::timerWorker, this);
    timerThread.detach();

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
            else if (mod == 3) gpr[a] = gpr[b] / gpr[c];
            else throw std::runtime_error("Invalid arithmetic modifier");
            break;

        case 0x6: // not/and/or/xor
            if (mod == 0) gpr[a] = ~gpr[b];
            else if (mod == 1) gpr[a] = gpr[b] & gpr[c];
            else if (mod == 2) gpr[a] = gpr[b] | gpr[c];
            else if (mod == 3) gpr[a] = gpr[b] ^ gpr[c];
            else throw std::runtime_error("Invalid logic modifier");
            break;

        case 0x7: // shl/shr
            if (mod == 0) gpr[a] = gpr[b] << gpr[c];
            else if (mod == 1) gpr[a] = gpr[b] >> gpr[c];
            else throw std::runtime_error("Invalid shift modifier");
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
                throw std::runtime_error("Invalid store modifier");
            }
            break;
        case 0x9: // load / csr
            if (mod == 0x0) {
                gpr[a] = csr[b];
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
                csr[a] = gpr[b];
            }
            else if (mod == 0x5) {
                csr[a] = csr[b] | d;
            }
            else if (mod == 0x6) {
                csr[a] = read32(gpr[b] + gpr[c] + d);
            }
            else if (mod == 0x7) {
                csr[a] = read32(gpr[b]);
                gpr[b] = gpr[b] + d;
            }
            else if (mod == 0x8){
                gpr[15] = read32(gpr[14]);
                gpr[14] += 4;

                csr[0] = read32(gpr[14]);
                gpr[14] += 4;
            }
            else {
                throw std::runtime_error("Invalid load modifier");
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
                throw std::runtime_error("Invalid call modifier");
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
                throw std::runtime_error("Invalid jump modifier");
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
        //TERMINAL
        if (terminalInterruptPending.load()) {

            std::cerr << "Trying terminal interrupt\n";

            bool globalMasked = csr[0] & (1 << 2);
            bool terminalMasked = csr[0] & (1 << 1);
            bool handlerSet = csr[1] != 0;

            std::cerr << "globalMasked=" << globalMasked << " terminalMasked=" << terminalMasked << "\n";

            if (handlerSet && !globalMasked && !terminalMasked) {
                terminalInterruptPending.store(false);
                enterInterrupt(3);
            }
        }

        //TIMER
        if (timerInterruptPending.load()) {
            bool globalMasked = csr[0] & (1 << 2);
            bool timerMasked = csr[0] & 1;
            bool handlerSet = csr[1] != 0;

            if (handlerSet && !globalMasked && !timerMasked) {
                timerInterruptPending.store(false);
                enterInterrupt(2);
            }
        }
                
    }
}

void Emulator::printRegisters() const
{
    std::cout << "Emulated processor state:\n";

    for (int i = 0; i < 16; i++) {
        std::cout << "r" << i << "=0x"
                  << std::hex << std::setw(8) << std::setfill('0')
                  << gpr[i];

        if (i % 4 == 3) {
            std::cout << "\n";
        } else {
            std::cout << " ";
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

void Emulator::terminalWorker()
{
    while (true) {
        char ch;
        std::cin.get(ch);

        if (ch == '\n') {
            continue;
        }

        terminalCharacter.store((uint8_t)ch);
        terminalInterruptPending.store(true);
    }
}

void Emulator::timerWorker()
{
    while (true) {
        uint32_t cfg = timerConfig.load();
        uint32_t period;

        switch (cfg) {
        case 0: period = 500; break;
        case 1: period = 1000; break;
        case 2: period = 1500; break;
        case 3: period = 2000; break;
        case 4: period = 5000; break;
        case 5: period = 10000; break;
        case 6: period = 30000; break;
        case 7: period = 60000; break;
        default: period = 500; break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(period));

        timerInterruptPending.store(true);
    }
}