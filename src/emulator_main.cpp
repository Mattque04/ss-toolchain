#include "../inc/emulator.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: ./emulator <input.hex>\n";
        return 1;
    }

    try {
        Emulator emulator;

        emulator.loadHexFile(argv[1]);
        emulator.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Emulator error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}