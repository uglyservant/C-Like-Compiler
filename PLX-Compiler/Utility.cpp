#include "stdafx.h"
#include "PLX.h"
#include "PLX-Compiler.h"
#include "PLX-CompilerDlg.h"

const char* RESERVED_WORD[RESERVED_WORD_SIZE] = {
	"main",		"const",	"var",		"procedure",
	"call",		"if",		"then",		"else",
	"while",	"do",		"read",		"write",
	"repeat",	"until",     "break",	"continue"
}; //15

const char SPECIAL_SYMBOLS[SPECIAL_SYM_SIZE] = {
	'+', '-', '*', '/',
	'(', ')', '=', ',',
	'!', '<', '>', ';',
	'%', '{', '}', '[',
	']'
}; //17

const char* TOKEN_STR[TOKEN_SIZE] = {
	"illegal_sym",	"main_sym",		  "ident_sym",	  "number_sym",		"plus_sym",
	"minus_sym",
	"mult_sym",		"slash_sym",	  "not_sym",		"mod_sym",		"eq_sym",
	"neq_sym",
	"les_sym",		"leq_sym",		  "gtr_sym",		"geq_sym",		"lparent_sym",
	"rparent_sym",
	"comma_sym",	"semicolon_sym",  "becomes_sym",	"lbrace_sym",	"rbrace_sym",
	"if_sym",
	"then_sym",		"while_sym",	  "do_sym",			"call_sym",		"const_sym",
	"var_sym",		"proc_sym",		  "write_sym",	    "read_sym",		"else_sym",
	"repeat_sym",	"until_sym",	  "break_sym",	    "continue_sym", "lbracket_sym",
	"rbracket_sym"
};

const char* PCODE_STR[P_CODE_SIZE] = {
	"illegal",
	"LIT", "OPR", "LOD", "STO", "CAL", "INT",
	"JMP", "JPC", "RED", "WRT", "STA", "LDA"
};

void AppendTextToEditCtrl(CEdit* edit, LPCTSTR pszText) {
	int nLength = edit->GetWindowTextLength();
	edit->SetSel(nLength, nLength);
	edit->ReplaceSel(pszText);
}