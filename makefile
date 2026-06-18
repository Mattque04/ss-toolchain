all: asembler linker

asembler: src/main.cpp src/assembler.cpp misc/parser.tab.c misc/lex.yy.c
	g++ -std=c++17 -Wall -Wextra -Iinc -Imisc \
		src/main.cpp src/assembler.cpp misc/parser.tab.c misc/lex.yy.c \
		-o asembler

misc/parser.tab.c misc/parser.tab.h: misc/parser.y
	bison -d -o misc/parser.tab.c misc/parser.y

misc/lex.yy.c: misc/lexer.l misc/parser.tab.h
	flex -o misc/lex.yy.c misc/lexer.l

clean:
	rm -f asembler linker misc/parser.tab.c misc/parser.tab.h misc/lex.yy.c

linker: src/linker_main.cpp src/linker.cpp

	g++ -std=c++17 -Wall -Wextra -Iinc \
		src/linker_main.cpp src/linker.cpp \
		-o linker