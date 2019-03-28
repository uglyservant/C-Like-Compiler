#pragma once

#ifndef PLX
#define PLX

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define SPECIAL_SYM_SIZE 17
#define RESERVED_WORD_SIZE 16
#define TOKEN_SIZE 40
#define SYMBOL_TABLE_SIZE 1024
#define P_CODE_SIZE 13
#define CODE_SIZE 1024
#define STACK_SIZE 256
#define LEX_LIST_SIZE 1024

#define IDENTIFIER_LENGTH 20
#define NUM_LENGTH 8
#define MAX_ADDRESS 1024
#define MAX_LEXICAL_LEVEL 3

#define	WHILE_DO 1
#define DO_WHILE 2

//词法分析数据结构
typedef enum {
	illegal_sym,	main_sym,			ident_sym,			number_sym,			plus_sym,
	minus_sym,
	mult_sym,		slash_sym,			not_sym,/*e*/		mod_sym/*e*/,		eq_sym,
	neq_sym,
	les_sym,		leq_sym,			gtr_sym,			geq_sym,			lparent_sym,
	rparent_sym,
	comma_sym,		semicolon_sym,		becomes_sym,		lbrace_sym,			rbrace_sym,
	if_sym,
	then_sym,		while_sym,			do_sym,				call_sym/*e*/,		const_sym/*e*/,
	var_sym,		proc_sym,			write_sym,			read_sym,			else_sym/*e*/,
	repeat_sym/*e*/,until_sym/*e*/,		break_sym/*e*/,		continue_sym,/*e*/	lbracket_sym,/*e*/
	rbracket_sym/*e*/
} token_t;

extern const char* TOKEN_STR[TOKEN_SIZE];
extern const char* RESERVED_WORD[RESERVED_WORD_SIZE];
extern const char SPECIAL_SYMBOLS[SPECIAL_SYM_SIZE];

typedef struct {
	token_t token;
	int value;
	char name[IDENTIFIER_LENGTH];
} lexelem;

extern lexelem lex_list[LEX_LIST_SIZE];
extern int lex_length;

//符号表数据结构
typedef enum {
	const_type = 1, var_type, proc_type, array_type
} symtype_t;

typedef struct {
	symtype_t type;
	char name[IDENTIFIER_LENGTH];
	int val;
	int level;
	int addr;
} sym;

extern sym sym_table[SYMBOL_TABLE_SIZE];
extern int sym_length;
/* 常数保存type, name, value
变量保存type, name, level, addr
过程保存type, name, level, addr */

//pcode数据结构
typedef enum {
	illegal_op,
	lit_op, opr_op, lod_op, sto_op, cal_op, int_op,
	jmp_op, jpc_op, red_op, wrt_op, sta_op, lda_op
} pcode_t;

typedef enum {
	ret_op, neg_op, add_op, sub_op, mul_op,
	div_op, not_op, mod_op, eql_op, neq_op,
	lss_op, leq_op, gtr_op, geq_op
} opr_t;

extern const char* PCODE_STR[P_CODE_SIZE];

typedef struct {
	pcode_t op;
	int l;
	int a;
} aop;

extern aop code[CODE_SIZE];
extern int code_length;
extern int errorcnt;

extern int stack[STACK_SIZE];

//主要的函数
void lex(FILE *ifp);
void compile();
void interpret(FILE *ifp);

#endif