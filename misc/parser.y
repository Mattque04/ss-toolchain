%code requires {
    #include <cstdint>
}

%{
#include <iostream>
#include <cstdlib>
#include <cstdint>
#include "../inc/assembler.hpp"

extern Assembler assembler;
extern int yylex();
void yyerror(const char* s);
%}

%union {
    uint32_t num;
    char* str;
}

%token <num> NUMBER
%token <num> GPR
%token <num> CSR
%token <str> IDENT
%token <str> STRING

%token GLOBAL EXTERN SECTION WORD SKIP END ASCII EQU
%token HALT INT IRET CALL RET JMP BEQ BNE BGT
%token PUSH POP XCHG ADD SUB MUL DIV NOT AND OR XOR SHL SHR
%token LD ST CSRRD CSRWR

%token COLON COMMA DOLLAR LBRACKET RBRACKET
%token PLUS MINUS STAR SLASH LPAREN RPAREN

%type <num> expr

%left PLUS MINUS
%left STAR SLASH
%precedence UMINUS

%%

program:
      lines
;

lines:
      lines line
    |
;

line:
      label
    | directive
    | instruction
;

label:
      IDENT COLON
      {
          assembler.defineLabel($1);
          free($1);
      }
;

directive:
      GLOBAL global_symbol_list
    | EXTERN extern_symbol_list
    | EQU IDENT COMMA expr
      {
          assembler.defineEqu($2, $4);
          free($2);
      }
    | SECTION IDENT
      {
          assembler.startSection($2);
          free($2);
      }
    | WORD init_list
    | ASCII STRING
      {
          assembler.emitAscii($2);
          free($2);
      }
    | SKIP NUMBER
      {
          assembler.emitSkip($2);
      }
    | END
      {
          YYACCEPT;
      }
;

global_symbol_list:
      IDENT
      {
          assembler.addGlobal($1);
          free($1);
      }
    | global_symbol_list COMMA IDENT
      {
          assembler.addGlobal($3);
          free($3);
      }
;

extern_symbol_list:
      IDENT
      {
          assembler.addExtern($1);
          free($1);
      }
    | extern_symbol_list COMMA IDENT
      {
          assembler.addExtern($3);
          free($3);
      }
;

init_list:
      init
    | init_list COMMA init
;

init:
      NUMBER
      {
          assembler.emitWord($1);
      }
    | IDENT
      {
          assembler.emitWordSymbol($1);
          free($1);
      }
;

expr:
      NUMBER
      {
          $$ = $1;
      }
    | IDENT
      {
          $$ = assembler.getAbsoluteSymbolValue($1);
          free($1);
      }
    | expr PLUS expr
      {
          $$ = $1 + $3;
      }
    | expr MINUS expr
      {
          $$ = $1 - $3;
      }
    | expr STAR expr
      {
          $$ = $1 * $3;
      }
    | expr SLASH expr
      {
          if ($3 == 0) {
              yyerror("division by zero in .equ expression");
              YYERROR;
          }
          $$ = $1 / $3;
      }
    | MINUS expr %prec UMINUS
      {
          $$ = (uint32_t)(-(int32_t)$2);
      }
    | LPAREN expr RPAREN
      {
          $$ = $2;
      }
;

instruction:
      HALT
      {
          assembler.emitHalt();
      }
    | INT
      {
          assembler.emitInt();
      }
    | IRET
      {
          assembler.emitIret();
      }
    | RET
      {
          assembler.emitRet();
      }

    | CALL NUMBER
      {
          assembler.emitCallLiteral((int32_t)$2);
      }
    | JMP NUMBER
      {
          assembler.emitJmpLiteral((int32_t)$2);
      }
    | BEQ GPR COMMA GPR COMMA NUMBER
      {
          assembler.emitBeqLiteral($2, $4, (int32_t)$6);
      }
    | BNE GPR COMMA GPR COMMA NUMBER
      {
          assembler.emitBneLiteral($2, $4, (int32_t)$6);
      }
    | BGT GPR COMMA GPR COMMA NUMBER
      {
          assembler.emitBgtLiteral($2, $4, (int32_t)$6);
      }

    | CALL IDENT
      {
          assembler.emitCallSymbol($2);
          free($2);
      }
    | JMP IDENT
      {
          assembler.emitJmpSymbol($2);
          free($2);
      }
    | BEQ GPR COMMA GPR COMMA IDENT
      {
          assembler.emitBeqSymbol($2, $4, $6);
          free($6);
      }
    | BNE GPR COMMA GPR COMMA IDENT
      {
          assembler.emitBneSymbol($2, $4, $6);
          free($6);
      }
    | BGT GPR COMMA GPR COMMA IDENT
      {
          assembler.emitBgtSymbol($2, $4, $6);
          free($6);
      }

    | PUSH GPR
      {
          assembler.emitPush($2);
      }
    | POP GPR
      {
          assembler.emitPop($2);
      }
    | XCHG GPR COMMA GPR
      {
          assembler.emitXchg($2, $4);
      }

    | ADD GPR COMMA GPR
      {
          assembler.emitAdd($2, $4);
      }
    | SUB GPR COMMA GPR
      {
          assembler.emitSub($2, $4);
      }
    | MUL GPR COMMA GPR
      {
          assembler.emitMul($2, $4);
      }
    | DIV GPR COMMA GPR
      {
          assembler.emitDiv($2, $4);
      }

    | NOT GPR
      {
          assembler.emitNot($2);
      }
    | AND GPR COMMA GPR
      {
          assembler.emitAnd($2, $4);
      }
    | OR GPR COMMA GPR
      {
          assembler.emitOr($2, $4);
      }
    | XOR GPR COMMA GPR
      {
          assembler.emitXor($2, $4);
      }

    | SHL GPR COMMA GPR
      {
          assembler.emitShl($2, $4);
      }
    | SHR GPR COMMA GPR
      {
          assembler.emitShr($2, $4);
      }

    | LD DOLLAR NUMBER COMMA GPR
      {
          assembler.emitLdImmediateLiteral((int32_t)$3, $5);
      }
    | LD DOLLAR IDENT COMMA GPR
      {
          assembler.emitLdImmediateSymbol($3, $5);
          free($3);
      }
    | LD GPR COMMA GPR
      {
          assembler.emitLdRegister($2, $4);
      }
    | LD IDENT COMMA GPR
      {
          assembler.emitLdMemSymbol($2, $4);
          free($2);
      }
    | LD LBRACKET GPR RBRACKET COMMA GPR
      {
          assembler.emitLdMemReg($3, $6);
      }
    | LD LBRACKET GPR PLUS NUMBER RBRACKET COMMA GPR
      {
          assembler.emitLdMemRegLiteral($3, (int32_t)$5, $8);
      }
    | LD LBRACKET GPR PLUS IDENT RBRACKET COMMA GPR
      {
          assembler.emitLdMemRegSymbol($3, $5, $8);
          free($5);
      }

    | ST GPR COMMA IDENT
      {
          assembler.emitStMemSymbol($2, $4);
          free($4);
      }
    | ST GPR COMMA LBRACKET GPR RBRACKET
      {
          assembler.emitStMemReg($2, $5);
      }
    | ST GPR COMMA LBRACKET GPR PLUS NUMBER RBRACKET
      {
          assembler.emitStMemRegLiteral($2, $5, (int32_t)$7);
      }
    | ST GPR COMMA LBRACKET GPR PLUS IDENT RBRACKET
      {
          assembler.emitStMemRegSymbol($2, $5, $7);
          free($7);
      }

    | CSRRD CSR COMMA GPR
      {
          assembler.emitCsrrd($2, $4);
      }
    | CSRWR GPR COMMA CSR
      {
          assembler.emitCsrwr($2, $4);
      }
;

%%

void yyerror(const char* s) {
    std::cerr << "Parse error: " << s << std::endl;
}