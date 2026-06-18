#include "../inc/assembler.hpp"

#include <cstdio>
#include <iostream>
#include <string>

extern int yyparse();
extern FILE* yyin;

Assembler assembler;

int main(int argc, char** argv)
{
    std::string inputFile;
    std::string outputFile;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Missing output file after -o\n";
                return 1;
            }
            outputFile = argv[++i];
        } else {
            inputFile = arg;
        }
    }

    if (inputFile.empty() || outputFile.empty()) {
        std::cerr << "Usage: ./asembler -o <output_file> <input_file>\n";
        return 1;
    }

    yyin = fopen(inputFile.c_str(), "r");
    if (!yyin) {
        std::cerr << "Cannot open input file: " << inputFile << "\n";
        return 1;
    }

    int result = yyparse();

    fclose(yyin);

    if (result != 0) {
        std::cerr << "Assembly failed\n";
        return 1;
    }

    assembler.printState();
    assembler.writeObjectFile(outputFile);

    return 0;
}