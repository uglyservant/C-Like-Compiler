#include "stdafx.h"
#include "PLX.h"
#include "PLX-Compiler.h"
#include "PLX-CompilerDlg.h"

//递归下降函数
//lexlevel: 层次
//symtbl_index: 符号表下标
void program();
void block(int lexlevel, int symtbl_index);
void constDeclaration(int lexlevel, int *ptr_symtbl_index, int *ptr_stack_index);
void varDeclaration(int lexlevel, int *ptr_symtbl_index, int *ptr_stack_index);
void statement(int lexlevel, int *ptr_symtbl_index, int *current_break, int *current_continue);
void condition(int lexlevel, int *ptr_symtbl_index);
void expression(int lexlevel, int *ptr_symtbl_index);
void term(int lexlevel, int *ptr_symtbl_index);
void factor(int lexlevel, int *ptr_symtbl_index);
void identifier(int lexlevel, int *ptr_symtbl_index);

//工具函数
void gen(pcode_t op, int lv, int addr); //保存指令
void enter(symtype_t t, int *ptr_symtbl_index, int *ptr_stack_index, int lexlevel); //保存符号表
void error(int errcase); //报错

int nextToken(); //读取下一个token
int position(char *ident, int *ptr_symtb_index, int lexlevel); //定位某个标识符

//变量定义
sym sym_table[SYMBOL_TABLE_SIZE];

aop code[CODE_SIZE];

int token, num, type;

int errorcnt = 0;
int sym_length = 0, code_length = 0;
int lex_index = 0, code_index = 0, symtbl_index = 0;

char ident[IDENTIFIER_LENGTH];

void compile() {
	errorcnt = 0;
	lex_index = 0;
	code_index = 0;
	symtbl_index = 0;

	program();
	if (errorcnt > 0)
		return;

	sym_length = symtbl_index;
	code_length = code_index;
}

//<程序> ::= <分程序>.
void program() {
	token = nextToken();
	if (token != main_sym) {
		error(30);   // 缺少 main 函数
		return;
	}

	token = nextToken();
	block(0, 0);
	if (errorcnt > 0)
		return;
	if (token != illegal_sym) {
		error(9);
		return;
	}
}

//<分程序> ::= [<常量说明部分>][<变量说明部分>][<过程说明部分>]<语句>
void block(int lexlevel, int symtbl_index) {
	if (lexlevel > MAX_LEXICAL_LEVEL) {
		error(26);
		return;
	}

	//stack_index: 数据在栈中的位置
	int stack_index = 3;//给sp, bp, pc, ret预留空间

	int symtbl_index0 = symtbl_index;

	sym_table[symtbl_index].addr = code_index;
	gen(jmp_op, 0, 0);  //分程序入口的jump地址还没有确定

	if (token != lbrace_sym) {
		error(28);
		return;
	}

	token = nextToken();
	//[<常量说明部分>] ::= [const<常量定义>{,<常量定义>}];
	while (token == const_sym) {
		token = nextToken();

		constDeclaration(lexlevel, &symtbl_index, &stack_index);
		if (errorcnt > 0)
			return;

		while (token == comma_sym) {
			token = nextToken();

			constDeclaration(lexlevel, &symtbl_index, &stack_index);
			if (errorcnt > 0)
				return;
		}

		if (token == semicolon_sym)
			token = nextToken();
		else {
			error(5);
			return;
		}
	}

	//[<变量说明部分>]::= [var<标识符>{,<标识符>}];
	while (token == var_sym) {
		token = nextToken();

		varDeclaration(lexlevel, &symtbl_index, &stack_index);
		if (errorcnt > 0)
			return;

		while (token == comma_sym) {
			token = nextToken();

			varDeclaration(lexlevel, &symtbl_index, &stack_index);
			if (errorcnt > 0)
				return;
		}

		if (token == semicolon_sym)
			token = nextToken();
		else {
			error(5);
			return;
		}
	}

	//[<过程说明部分>] ::= [<过程首部><分程序>;{<过程说明部分>}]
	//等效于{<过程首部><分程序>;}
	while (token == proc_sym) {
		token = nextToken();

		if (token == ident_sym) {
			enter(proc_type, &symtbl_index, &stack_index, lexlevel); // 此时level还是本层的level
			token = nextToken();
		}
		else {
			error(4);
			return;
		}

		//if (token == semicolon_sym)
		//	token = nextToken();
		//else {
		//	error(5);
		//	return;
		//}

		block(lexlevel + 1, symtbl_index);
		if (errorcnt > 0)
			return;

		//if (token == semicolon_sym)
		//	token = nextToken();
		//else {
		//	error(5);
		//	return;
		//}
	}

	//jump的地址现在确定了，地址回填
	code[sym_table[symtbl_index0].addr].a = code_index;
	sym_table[symtbl_index0].addr = code_index;

	gen(int_op, 0, stack_index);

	int break_index = 0;
	int continue_index = 0;

	while (token != rbrace_sym) {
		statement(lexlevel, &symtbl_index, &break_index, &continue_index);
		if (errorcnt > 0)
			return;
	}

	gen(opr_op, 0, ret_op);

	token = nextToken();
}

//<常量定义> ::= <标识符>=<无符号整数>
void constDeclaration(int lexlevel, int *ptr_symtbl_index, int *ptr_stack_index) {
	if (token == ident_sym) {
		token = nextToken();

		if (token == becomes_sym) {
			token = nextToken();

			if (token == minus_sym) {
				token = nextToken();
				num = -num;
			}

			if (token == number_sym) {
				enter(const_type, ptr_symtbl_index, ptr_stack_index, lexlevel);
				token = nextToken();
			}
			else {
				error(2);
				return;
			}
		}
		else {
			error(1);
			return;
		}
	}
	else {
		error(4);
		return;
	}
}

//<变量定义>::= <标识符>;
void varDeclaration(int lexlevel, int *ptr_symtbl_index, int *ptr_stack_index) {
	if (token == ident_sym) {
		enter(var_type, ptr_symtbl_index, ptr_stack_index, lexlevel);
		if (errorcnt > 0)
			return;
		token = nextToken();
	}
	else {
		error(4);
		return;
	}
}

//<语句> ::= <赋值语句>|<条件语句>|<当型循环语句>|<过程调用语句>|<读语句>|<写语句>|<复合语句>|<重复语句>|<空>
void statement(int lexlevel, int *ptr_symtbl_index, int *current_break, int *current_continue) {
	if (errorcnt > 0) return;

	int i, code_ind1, code_ind2;

	//<空语句> ::= ;
	if (token == semicolon_sym) {
		token = nextToken();
		return;
	}

	//<循环结束语句> ::= break
	else if (token == break_sym) {
		*current_break = code_index;
		gen(jmp_op, 0, 0);

		token = nextToken();
		if (token == semicolon_sym) {
			token = nextToken();
		}
		else {
			error(10); // without semicolon;
		}
	}

	//<循环直接进入下一轮> ::= continue
	else if (token == continue_sym) {
		*current_continue = code_index;
		gen(jmp_op, 0, 0);

		token = nextToken();
		if (token == semicolon_sym)
			token = nextToken();
		else
			error(10); // without semicolon;
	}

	//<过程调用语句> ::= call<标识符>
	else if (token == call_sym) {
		if (errorcnt > 0)
			return;

		token = nextToken();
		if (token == ident_sym) {
			i = position(ident, ptr_symtbl_index, lexlevel);
			if (i == 0) {
				error(11);
				return;
			}
			else if (sym_table[i].type == proc_type) {
				gen(cal_op, lexlevel - sym_table[i].level, sym_table[i].addr);
				token = nextToken();
			}
			else {
				error(15);
				return;
			}

			if (token == semicolon_sym)
				token = nextToken();
			else {
				return;
				error(10);
			}
		}
		else {
			error(14);
			return;
		}
	}

	//<赋值语句> ::= <标识符>:=<表达式>
	else if (token == ident_sym) {
		i = position(ident, ptr_symtbl_index, lexlevel);

		if (i == 0) {
			error(11);
			return;
		}
		else if (sym_table[i].type == var_type) {
			token = nextToken();

			if (token == becomes_sym) {
				token = nextToken();

				expression(lexlevel, ptr_symtbl_index);
				if (errorcnt > 0)
					return;

				gen(sto_op, lexlevel - sym_table[i].level, sym_table[i].addr);

				if (token == semicolon_sym)
					token = nextToken();
				else {
					error(10);
					return;
				}
			}
			else if (token == lbracket_sym) {
				token = nextToken();

				expression(lexlevel, ptr_symtbl_index);
				if (errorcnt > 0)
					return;

				if (token != rbracket_sym) {
					error(28);
					return;
				}
				
				token = nextToken();

				if (token != becomes_sym) {
					error(13);
					return;
				}
				token = nextToken();

				expression(lexlevel, ptr_symtbl_index);
				if (errorcnt > 0)
					return;

				gen(sta_op, lexlevel - sym_table[i].level, sym_table[i].addr);
				
				if (token != semicolon_sym) {
					error(10);
					return;
				}
				token = nextToken();
			}
			else {
				error(13);
				return;
			}

		}
		else {
			error(12);
			i = 0;
		}
	}

	//<写语句> ::= write'('[<标识符>|<数>]{,[<标识符>|<数>]}')'
	else if (token == write_sym) {
		token = nextToken();

		if (token == lparent_sym) {
			do {
				token = nextToken();
				expression(lexlevel, ptr_symtbl_index);
				if (errorcnt > 0)
					return;
				gen(wrt_op, 0, 1);
			} while (token == comma_sym);

			if (token == rparent_sym) {
				token = nextToken();
				if (token == semicolon_sym) {
					token = nextToken();
				}
				else {
					error(10);
					return;
				}
			}
			else {
				error(28);
				return;
			}
		}
		else {
			error(28);
		}

	}

	//<读语句> ::= read'('<标识符>{,<标识符>}')'
	else if (token == read_sym) {
		token = nextToken();

		if (token == lparent_sym) {
			do {
				token = nextToken();
				if (token == ident_sym) {
					gen(red_op, 0, 0);

					i = position(ident, ptr_symtbl_index, lexlevel);
					if (i == 0) {
						error(11);
						return;
					}
					else if (sym_table[i].type != var_type) {
						error(12);
						i = 0;
						return;
					}
					gen(sto_op, lexlevel - sym_table[i].level, sym_table[i].addr);
					token = nextToken();
				}
				else {
					error(22);
					return;
				}

			} while (token == comma_sym);
			

			if (token == rparent_sym) {
				token = nextToken();
				if (token == semicolon_sym) {
					token = nextToken();
				}
				else {
					error(10);
					return;
				}
			}
			else {
				error(28);
				return;
			}
		}
		else {
			error(28);
			return;
		}
	}

	//<重复语句> ::= repeat<语句>{;<语句>}until<条件>
	else if (token == repeat_sym) {
		int break_index = 0;
		int continue_index = 0;

		code_ind1 = code_index;

		token = nextToken();
		statement(lexlevel, ptr_symtbl_index, &break_index, &continue_index);
		if (errorcnt > 0)
			return;
		
		code_ind2 = code_index;
		if (token == until_sym) {
			token = nextToken();
			condition(lexlevel, ptr_symtbl_index);
			if (errorcnt > 0)
				return;
			gen(jpc_op, 0, code_ind1);

			if (token == semicolon_sym)
				token = nextToken();
			else {
				error(10);
				return;
			}
		}
		else {
			error(27);
			return;
		}

		if (break_index != 0)
			code[break_index].a = code_index;
		if (continue_index != 0)
			code[continue_index].a = code_ind2;
	}

	//<循环语句> ::= do <语句> while <条件>
	else if (token == do_sym) {
		int break_index = 0;
		int continue_index = 0;

		code_ind1 = code_index;

		token = nextToken();
		statement(lexlevel, ptr_symtbl_index, &break_index, &continue_index);
		if (errorcnt > 0)
			return;

		code_ind2 = code_index;
		if (token == while_sym) {
			token = nextToken();
			condition(lexlevel, ptr_symtbl_index);
			if (errorcnt > 0)
				return;
			gen(jpc_op, 0, code_index+2);
			gen(jmp_op, 0, code_ind1);

			if (token == semicolon_sym)
				token = nextToken();
			else {
				error(10);
				return;
			}
		}
		else {
			error(29);
			return;
		}

		if (break_index != 0)
			code[break_index].a = code_index;
		if (continue_index != 0)
			code[continue_index].a = code_ind2;

	}

	//<条件语句> ::= if <条件> then <语句> [else <语句>]
	else if (token == if_sym) {
		token = nextToken();

		condition(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;

		if (token == then_sym)
			token = nextToken();

		else {
			error(16);
			return;
		}

		code_ind1 = code_index;
		gen(jpc_op, 0, 0); //等待设置 jpc 的位置

		statement(lexlevel, ptr_symtbl_index, current_break, current_continue);
		if (errorcnt > 0)
			return;

		if (token == else_sym) {
			token = nextToken();

			code[code_ind1].a = code_index + 1; //设置 jpc 位置
			code_ind1 = code_index;
			gen(jmp_op, 0, 0); //等待设置 jmp 的位置

			statement(lexlevel, ptr_symtbl_index, current_break, current_continue);
			if (errorcnt > 0)
				return;

			code[code_ind1].a = code_index; // 回填地址
		}

		code[code_ind1].a = code_index; //设置 jpc 或 jmp 地址
	}

	//<当型循环语句> ::= while<条件>do<语句>
	else if (token == while_sym) {
		int break_index = 0;
		int continue_index = 0;

		code_ind1 = code_index;

		token = nextToken();
		condition(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;

		code_ind2 = code_index;
		gen(jpc_op, 0, 0); // 等待设置 jpc 位置

		if (token == do_sym)
			token = nextToken();
		else {
			error(18);
			return;
		}

		statement(lexlevel, ptr_symtbl_index, &break_index, &continue_index);
		if (errorcnt > 0)
			return;

		gen(jmp_op, 0, code_ind1);
		code[code_ind2].a = code_index; // 设置 jpc 位置

		if (break_index != 0)
			code[break_index].a = code_index;
		if (continue_index != 0)
			code[continue_index].a = code_ind1;
 	}

	//<复合语句> ::= begin<语句>{;<语句>}end
	else if (token == lbrace_sym) {
		token = nextToken();

		while (token != rbrace_sym) {
			statement(lexlevel, ptr_symtbl_index, current_break, current_continue);
			if (errorcnt > 0) return;
		}
		token = nextToken();
	}

	//<没有语句> ::= 错误
	else {
		error(7);
		return;
	}

}

//<条件> ::= <表达式><关系运算符><表达式>|odd<表达式>
void condition(int lexlevel, int *ptr_symtbl_index) {
	int relation;

	if (token == lparent_sym)
		token = nextToken();
	else {
		error(28);
		return;
	}

	if (token == not_sym) {
		token = nextToken();

		expression(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;

		gen(opr_op, 0, not_op);
	}
	else {
		expression(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;

		if ((token != eq_sym) && (token != neq_sym) && (token != les_sym) && (token != leq_sym) && (token != gtr_sym) && (token != geq_sym)) {
			error(20);
			return;
		}
		else {
			relation = token;
			token = nextToken();

			expression(lexlevel, ptr_symtbl_index);
			if (errorcnt > 0)
				return;

			switch (relation) {
				case eq_sym:
					gen(opr_op, 0, eql_op);
					break;
				case neq_sym:
					gen(opr_op, 0, neq_op);
					break;
				case les_sym:
					gen(opr_op, 0, lss_op);
					break;
				case leq_sym:
					gen(opr_op, 0, leq_op);
					break;
				case gtr_sym:
					gen(opr_op, 0, gtr_op);
					break;
				case geq_sym:
					gen(opr_op, 0, geq_op);
					break;
			}
		}
	}

	if (token == rparent_sym) {
		token = nextToken();
	}
	else {
		error(28);
		return;
	}
}

//<表达式> ::= [+|-]<项>{<加法运算符><项>}
void expression(int lexlevel, int *ptr_symtbl_index) {
	int addop;

	if (token == plus_sym || token == minus_sym) {
		addop = token;

		token = nextToken();
		term(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;
		if (addop == minus_sym)
			gen(opr_op, 0, neg_op);
	}
	else {
		term(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;
	}
	while (token == plus_sym || token == minus_sym) {
		addop = token;

		token = nextToken();
		term(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;
		if (addop == plus_sym)
			gen(opr_op, 0, add_op);
		else
			gen(opr_op, 0, sub_op);
	}
}

//<项> ::= <因子>{<乘法运算符><因子>}
void term(int lexlevel, int *ptr_symtbl_index) {
	int mulop;

	factor(lexlevel, ptr_symtbl_index);
	if (errorcnt > 0)
		return;

	while (token == mult_sym || token == slash_sym || token == mod_sym) {
		mulop = token;

		token = nextToken();
		factor(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;

		if (mulop == mult_sym)
			gen(opr_op, 0, mul_op);
		else if (mulop == slash_sym)
			gen(opr_op, 0, div_op);
		else
			gen(opr_op, 0, mod_op);
	}
}

//<因子> ::= <标识符>|<无符号整数>|'('<表达式>')'
void factor(int lexlevel, int *ptr_symtbl_index) {
		//<标识符>
		if (token == ident_sym) {
			identifier(lexlevel, ptr_symtbl_index);
			if (errorcnt > 0)
				return;
		}

		//<无符号整数> ::= <数字>{<数字>}
		else if (token == number_sym) {
			if (num >= MAX_ADDRESS) {
				error(25);
				num = 0;
				return;
			}
			gen(lit_op, 0, num);
			token = nextToken();
		}

		//'('<表达式>')'
		else if (token == lparent_sym) {
			token = nextToken();
			expression(lexlevel, ptr_symtbl_index);
			if (errorcnt > 0)
				return;
			if (token == rparent_sym)
				token = nextToken();
			else {
				error(22);
				return;
			}
		}
		
		else {
			error(23);
			return;
		}
	
}

//<标识符> ::= <字母>{<字母>|<数字>}
void identifier(int lexlevel, int *ptr_symtbl_index) {
	int i, type, level, adr, val;
	i = position(ident, ptr_symtbl_index, lexlevel);
	if (i == 0){
		error(11);
		return;
	}
	else {
		type = sym_table[i].type;
		level = sym_table[i].level;
		adr = sym_table[i].addr;
		val = sym_table[i].val;

		if (type == const_type)
			gen(lit_op, 0, val);
		else if (type == var_type)	{
			if (lex_list[lex_index].token == lbracket_sym) {
				token = nextToken();  //lbracket;

				token = nextToken();

				expression(lexlevel, ptr_symtbl_index);
				if (errorcnt > 0)
					return;

				if (token != rbracket_sym) {
					error(28);
					return;
				}

				gen(lda_op, lexlevel - level, adr);
				//token = nextToken();
			}
			else {
				gen(lod_op, lexlevel - level, adr);
			}
		}
		else {
			error(21);
			return;
		}
	}
	token = nextToken();
}


//*****************************
//********* 工具函数 ***********
//*****************************

void gen(pcode_t op, int lv, int addr) {
	if (code_index > CODE_SIZE)
		AppendTextToEditCtrl(editError, L"Program too long!");
	else {
		code[code_index].op = op;
		code[code_index].l = lv;
		code[code_index].a = addr;
		code_index++;
	}
}

//保存符号到符号表
void enter(symtype_t t, int *ptr_symtbl_index, int *ptr_stack_index, int lexlevel) {

	(*ptr_symtbl_index)++;
	strcpy_s(sym_table[*ptr_symtbl_index].name, ident);

	sym_table[*ptr_symtbl_index].type = t;
	if (t == const_type) {
		sym_table[*ptr_symtbl_index].val = num;
	}

	else if (t == var_type) {
		if (lex_list[lex_index].token == lbracket_sym) {
			token = nextToken(); // lbracket

			token = nextToken();
			if (token == number_sym) {
				sym_table[*ptr_symtbl_index].level = lexlevel;
				sym_table[*ptr_symtbl_index].addr = *ptr_stack_index;

				if (num <= 0) {
					error(31);
					return;
				}
				sym_table[*ptr_symtbl_index].val = num;
				(*ptr_stack_index) += num;
			}
			else if (token == ident_sym) {
				int i = position(ident, ptr_symtbl_index, lexlevel);
				if (i != 0) {
					if (sym_table[i].type == const_type) {
						sym_table[*ptr_symtbl_index].level = lexlevel;
						sym_table[*ptr_symtbl_index].addr = *ptr_stack_index;

						if (sym_table[i].val <= 0) {
							error(31);
							return;
						}
						sym_table[*ptr_symtbl_index].val = sym_table[i].val;
						(*ptr_stack_index) += sym_table[i].val;
					}
					else {
						error(30);
						return;
					}
				}
				else {
					error(11);
					return;
				}
			}
			else {
				error(30);
				return;
			}

			token = nextToken();
			if (token != rbracket_sym) {
				error(22);
				return;
			}
		}
		else {
			sym_table[*ptr_symtbl_index].level = lexlevel;
			sym_table[*ptr_symtbl_index].addr = *ptr_stack_index;
			(*ptr_stack_index)++;
		}
	}

	else {//procedure
		sym_table[*ptr_symtbl_index].level = lexlevel;
	}
}

//取出 lex 表的下一个 lex，如果是变量名将变量名放在 ident 中，如果是数字将 值放在 num 中
int nextToken() {
	token = lex_list[lex_index].token;
	if (token == ident_sym)
		strcpy_s(ident, lex_list[lex_index].name);
	else if (token == number_sym)
		num = lex_list[lex_index].value;

	lex_index += 1;
	return token;
}

//遇到某个符号时，找到符号表中，当前层次以外的，最靠里层的可用的符号定义
int position(char *ident, int *ptr_symtbl_index, int lexlevel) {
	int i = *ptr_symtbl_index;
	strcpy_s(sym_table[0].name, ident);

	while (strcmp(sym_table[i].name, ident)) {
		i--;
	}
	return i;
}

//错误处理
void error(int errcase) {
	errorcnt++;
	CString errstr;
	errstr.Format(L"Error %d at lex %d:\r\n", errcase, lex_index - 1);
	AppendTextToEditCtrl(editError, errstr);
	switch (errcase) {
	case 1:
		AppendTextToEditCtrl(editError, L"使用 '=' 赋值 \r\n");
		break;
	case 2:
		AppendTextToEditCtrl(editError, L"用常数对 'const' 变量赋值\r\n");
		break;
	case 3:
		AppendTextToEditCtrl(editError, L"标识符后面必须跟 '=' \r\n");
		break;
	case 4:
		AppendTextToEditCtrl(editError, L"const, var, procedure后面必须跟标识符\r\n");
		break;
	case 5:
		AppendTextToEditCtrl(editError, L"分号或逗号缺失\r\n");
		break;
	case 6:
		AppendTextToEditCtrl(editError, L"过程定义后面跟的符号不对\r\n");
		break;
	case 7:
		AppendTextToEditCtrl(editError, L"此处需要语句或主过程缺失");
		break;
	case 8:
		AppendTextToEditCtrl(editError, L"子程序的语句部分后面跟的符号不对\r\n");
		break;
	case 9:
		AppendTextToEditCtrl(editError, L"复合语句需要包含在 { 与 } 之间\r\n");
		break;
	case 10:
		AppendTextToEditCtrl(editError, L"分号缺失\r\n");
		break;
	case 11:
		AppendTextToEditCtrl(editError, L"标识符没有定义\r\n");
		break;
	case 12:
		AppendTextToEditCtrl(editError, L"不能对常量或过程赋值\r\n");
		break;
	case 13:
		AppendTextToEditCtrl(editError, L"此处需要赋值符号\r\n");
		break;
	case 14:
		AppendTextToEditCtrl(editError, L"call后面必须跟标识符");
		break;
	case 15:
		AppendTextToEditCtrl(editError, L"不能调用常量或变量\r\n");
		break;
	case 16:
		AppendTextToEditCtrl(editError, L"此处需要then");
		break;
	case 17:
		AppendTextToEditCtrl(editError, L"缺少end");
		break;
	case 18:
		AppendTextToEditCtrl(editError, L"此处需要do");
		break;
	case 19:
		AppendTextToEditCtrl(editError, L"语句后跟的符号不对\r\n");
		break;
	case 20:
		AppendTextToEditCtrl(editError, L"此处需要关系运算符\r\n");
		break;
	case 21:
		AppendTextToEditCtrl(editError, L"表达式里不能出现过程\r\n");
		break;
	case 22:
		AppendTextToEditCtrl(editError, L"右括号缺失\r\n");
		break;
	case 23:
		AppendTextToEditCtrl(editError, L"因子不能由这个符号开始\r\n");
		break;
	case 24:
		AppendTextToEditCtrl(editError, L"表达式不能由这个符号开始\r\n");
		break;
	case 25:
		AppendTextToEditCtrl(editError, L"数字过大\r\n");
		break;
	case 26:
		AppendTextToEditCtrl(editError, L"层次数超过限制\r\n");
		break;
	case 27:
		AppendTextToEditCtrl(editError, L"repeat语句缺少until\r\n");
		break;
	case 28:
		AppendTextToEditCtrl(editError, L"缺少括号\r\n");
		break;
	case 29:
		AppendTextToEditCtrl(editError, L"缺少 while\r\n");
		break;
	case 30:
		AppendTextToEditCtrl(editError, L"缺少 main\r\n");
		break;
	case 31:
		AppendTextToEditCtrl(editError, L"使用正常数对数组进行定义\r\n");
		break;
	default:
		AppendTextToEditCtrl(editError, L"未知错误\r\n");
		break;
	}
}