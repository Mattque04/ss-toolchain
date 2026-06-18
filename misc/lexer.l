%{
#include "parser.tab.h"
#include <cstdlib>
#include <cstring>
#include <cstdint>
%}

%%

"#".*                   ;

".global"               { return GLOBAL; }
".extern"               { return EXTERN; }
".section"              { return SECTION; }
".word"                 { return WORD; }
".skip"                 { return SKIP; }
".end"                  { return END; }
".ascii"                { return ASCII; }
".equ"                  { return EQU; }

"halt"                  { return HALT; }
"int"                   { return INT; }
"iret"                  { return IRET; }
"call"                  { return CALL; }
"ret"                   { return RET; }
"jmp"                   { return JMP; }
"beq"                   { return BEQ; }
"bne"                   { return BNE; }
"bgt"                   { return BGT; }
"push"                  { return PUSH; }
"pop"                   { return POP; }
"xchg"                  { return XCHG; }
"add"                   { return ADD; }
"sub"                   { return SUB; }
"mul"                   { return MUL; }
"div"                   { return DIV; }
"not"                   { return NOT; }
"and"                   { return AND; }
"or"                    { return OR; }
"xor"                   { return XOR; }
"shl"                   { return SHL; }
"shr"                   { return SHR; }
"ld"                    { return LD; }
"st"                    { return ST; }
"csrrd"                 { return CSRRD; }
"csrwr"                 { return CSRWR; }

"%sp"                   { yylval.num = 14; return GPR; }
"%pc"                   { yylval.num = 15; return GPR; }
"%r"[0-9]+              { yylval.num = static_cast<uint32_t>(strtoul(yytext + 2, nullptr, 10)); return GPR; }

"%status"               { yylval.num = 0; return CSR; }
"%handler"              { yylval.num = 1; return CSR; }
"%cause"                { yylval.num = 2; return CSR; }

"0x"[0-9a-fA-F]+        {
                            yylval.num = static_cast<uint32_t>(strtoul(yytext, nullptr, 16));
                            return NUMBER;
                        }

[0-9]+                  {
                            yylval.num = static_cast<uint32_t>(strtoul(yytext, nullptr, 10));
                            return NUMBER;
                        }

\"([^\"\n])*\"          {
                            size_t len = yyleng - 2;
                            char* s = (char*)malloc(len + 1);
                            memcpy(s, yytext + 1, len);
                            s[len] = '\0';
                            yylval.str = s;
                            return STRING;
                        }

[a-zA-Z_][a-zA-Z0-9_]*  {
                            yylval.str = strdup(yytext);
                            return IDENT;
                        }

":"                     { return COLON; }
","                     { return COMMA; }
"$"                     { return DOLLAR; }
"["                     { return LBRACKET; }
"]"                     { return RBRACKET; }
"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return STAR; }
"/"                     { return SLASH; }
"("                     { return LPAREN; }
")"                     { return RPAREN; }

[ \t\r\n]+              ;

.                       { return yytext[0]; }

%%

int yywrap() {
    return 1;
}