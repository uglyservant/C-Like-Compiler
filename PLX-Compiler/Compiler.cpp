#include "stdafx.h"
#include "PLX.h"
#include "PLX-Compiler.h"
#include "PLX-CompilerDlg.h"

//�ݹ��½�����
//lexlevel: ���
//symtbl_index: ���ű��±�
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

//���ߺ���
void gen(pcode_t op, int lv, int addr); //����ָ��
void enter(symtype_t t, int *ptr_symtbl_index, int *ptr_stack_index, int lexlevel); //������ű�
void error(int errcase); //����

int nextToken(); //��ȡ��һ��token
int position(char *ident, int *ptr_symtb_index, int lexlevel); //��λĳ����ʶ��

//��������
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

//<����> ::= <�ֳ���>.
void program() {
	token = nextToken();
	if (token != main_sym) {
		error(30);   // ȱ�� main ����
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

//<�ֳ���> ::= [<����˵������>][<����˵������>][<����˵������>]<���>
void block(int lexlevel, int symtbl_index) {
	if (lexlevel > MAX_LEXICAL_LEVEL) {
		error(26);
		return;
	}

	//stack_index: ������ջ�е�λ��
	int stack_index = 3;//��sp, bp, pc, retԤ���ռ�

	int symtbl_index0 = symtbl_index;

	sym_table[symtbl_index].addr = code_index;
	gen(jmp_op, 0, 0);  //�ֳ�����ڵ�jump��ַ��û��ȷ��

	if (token != lbrace_sym) {
		error(28);
		return;
	}

	token = nextToken();
	//[<����˵������>] ::= [const<��������>{,<��������>}];
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

	//[<����˵������>]::= [var<��ʶ��>{,<��ʶ��>}];
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

	//[<����˵������>] ::= [<�����ײ�><�ֳ���>;{<����˵������>}]
	//��Ч��{<�����ײ�><�ֳ���>;}
	while (token == proc_sym) {
		token = nextToken();

		if (token == ident_sym) {
			enter(proc_type, &symtbl_index, &stack_index, lexlevel); // ��ʱlevel���Ǳ����level
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

	//jump�ĵ�ַ����ȷ���ˣ���ַ����
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

//<��������> ::= <��ʶ��>=<�޷�������>
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

//<��������>::= <��ʶ��>;
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

//<���> ::= <��ֵ���>|<�������>|<����ѭ�����>|<���̵������>|<�����>|<д���>|<�������>|<�ظ����>|<��>
void statement(int lexlevel, int *ptr_symtbl_index, int *current_break, int *current_continue) {
	if (errorcnt > 0) return;

	int i, code_ind1, code_ind2;

	//<�����> ::= ;
	if (token == semicolon_sym) {
		token = nextToken();
		return;
	}

	//<ѭ���������> ::= break
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

	//<ѭ��ֱ�ӽ�����һ��> ::= continue
	else if (token == continue_sym) {
		*current_continue = code_index;
		gen(jmp_op, 0, 0);

		token = nextToken();
		if (token == semicolon_sym)
			token = nextToken();
		else
			error(10); // without semicolon;
	}

	//<���̵������> ::= call<��ʶ��>
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

	//<��ֵ���> ::= <��ʶ��>:=<���ʽ>
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

	//<д���> ::= write'('[<��ʶ��>|<��>]{,[<��ʶ��>|<��>]}')'
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

	//<�����> ::= read'('<��ʶ��>{,<��ʶ��>}')'
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

	//<�ظ����> ::= repeat<���>{;<���>}until<����>
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

	//<ѭ�����> ::= do <���> while <����>
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

	//<�������> ::= if <����> then <���> [else <���>]
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
		gen(jpc_op, 0, 0); //�ȴ����� jpc ��λ��

		statement(lexlevel, ptr_symtbl_index, current_break, current_continue);
		if (errorcnt > 0)
			return;

		if (token == else_sym) {
			token = nextToken();

			code[code_ind1].a = code_index + 1; //���� jpc λ��
			code_ind1 = code_index;
			gen(jmp_op, 0, 0); //�ȴ����� jmp ��λ��

			statement(lexlevel, ptr_symtbl_index, current_break, current_continue);
			if (errorcnt > 0)
				return;

			code[code_ind1].a = code_index; // �����ַ
		}

		code[code_ind1].a = code_index; //���� jpc �� jmp ��ַ
	}

	//<����ѭ�����> ::= while<����>do<���>
	else if (token == while_sym) {
		int break_index = 0;
		int continue_index = 0;

		code_ind1 = code_index;

		token = nextToken();
		condition(lexlevel, ptr_symtbl_index);
		if (errorcnt > 0)
			return;

		code_ind2 = code_index;
		gen(jpc_op, 0, 0); // �ȴ����� jpc λ��

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
		code[code_ind2].a = code_index; // ���� jpc λ��

		if (break_index != 0)
			code[break_index].a = code_index;
		if (continue_index != 0)
			code[continue_index].a = code_ind1;
 	}

	//<�������> ::= begin<���>{;<���>}end
	else if (token == lbrace_sym) {
		token = nextToken();

		while (token != rbrace_sym) {
			statement(lexlevel, ptr_symtbl_index, current_break, current_continue);
			if (errorcnt > 0) return;
		}
		token = nextToken();
	}

	//<û�����> ::= ����
	else {
		error(7);
		return;
	}

}

//<����> ::= <���ʽ><��ϵ�����><���ʽ>|odd<���ʽ>
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

//<���ʽ> ::= [+|-]<��>{<�ӷ������><��>}
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

//<��> ::= <����>{<�˷������><����>}
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

//<����> ::= <��ʶ��>|<�޷�������>|'('<���ʽ>')'
void factor(int lexlevel, int *ptr_symtbl_index) {
		//<��ʶ��>
		if (token == ident_sym) {
			identifier(lexlevel, ptr_symtbl_index);
			if (errorcnt > 0)
				return;
		}

		//<�޷�������> ::= <����>{<����>}
		else if (token == number_sym) {
			if (num >= MAX_ADDRESS) {
				error(25);
				num = 0;
				return;
			}
			gen(lit_op, 0, num);
			token = nextToken();
		}

		//'('<���ʽ>')'
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

//<��ʶ��> ::= <��ĸ>{<��ĸ>|<����>}
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
//********* ���ߺ��� ***********
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

//������ŵ����ű�
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

//ȡ�� lex �����һ�� lex������Ǳ����������������� ident �У���������ֽ� ֵ���� num ��
int nextToken() {
	token = lex_list[lex_index].token;
	if (token == ident_sym)
		strcpy_s(ident, lex_list[lex_index].name);
	else if (token == number_sym)
		num = lex_list[lex_index].value;

	lex_index += 1;
	return token;
}

//����ĳ������ʱ���ҵ����ű��У���ǰ�������ģ�����Ŀ��õķ��Ŷ���
int position(char *ident, int *ptr_symtbl_index, int lexlevel) {
	int i = *ptr_symtbl_index;
	strcpy_s(sym_table[0].name, ident);

	while (strcmp(sym_table[i].name, ident)) {
		i--;
	}
	return i;
}

//������
void error(int errcase) {
	errorcnt++;
	CString errstr;
	errstr.Format(L"Error %d at lex %d:\r\n", errcase, lex_index - 1);
	AppendTextToEditCtrl(editError, errstr);
	switch (errcase) {
	case 1:
		AppendTextToEditCtrl(editError, L"ʹ�� '=' ��ֵ \r\n");
		break;
	case 2:
		AppendTextToEditCtrl(editError, L"�ó����� 'const' ������ֵ\r\n");
		break;
	case 3:
		AppendTextToEditCtrl(editError, L"��ʶ���������� '=' \r\n");
		break;
	case 4:
		AppendTextToEditCtrl(editError, L"const, var, procedure����������ʶ��\r\n");
		break;
	case 5:
		AppendTextToEditCtrl(editError, L"�ֺŻ򶺺�ȱʧ\r\n");
		break;
	case 6:
		AppendTextToEditCtrl(editError, L"���̶��������ķ��Ų���\r\n");
		break;
	case 7:
		AppendTextToEditCtrl(editError, L"�˴���Ҫ����������ȱʧ");
		break;
	case 8:
		AppendTextToEditCtrl(editError, L"�ӳ������䲿�ֺ�����ķ��Ų���\r\n");
		break;
	case 9:
		AppendTextToEditCtrl(editError, L"���������Ҫ������ { �� } ֮��\r\n");
		break;
	case 10:
		AppendTextToEditCtrl(editError, L"�ֺ�ȱʧ\r\n");
		break;
	case 11:
		AppendTextToEditCtrl(editError, L"��ʶ��û�ж���\r\n");
		break;
	case 12:
		AppendTextToEditCtrl(editError, L"���ܶԳ�������̸�ֵ\r\n");
		break;
	case 13:
		AppendTextToEditCtrl(editError, L"�˴���Ҫ��ֵ����\r\n");
		break;
	case 14:
		AppendTextToEditCtrl(editError, L"call����������ʶ��");
		break;
	case 15:
		AppendTextToEditCtrl(editError, L"���ܵ��ó��������\r\n");
		break;
	case 16:
		AppendTextToEditCtrl(editError, L"�˴���Ҫthen");
		break;
	case 17:
		AppendTextToEditCtrl(editError, L"ȱ��end");
		break;
	case 18:
		AppendTextToEditCtrl(editError, L"�˴���Ҫdo");
		break;
	case 19:
		AppendTextToEditCtrl(editError, L"������ķ��Ų���\r\n");
		break;
	case 20:
		AppendTextToEditCtrl(editError, L"�˴���Ҫ��ϵ�����\r\n");
		break;
	case 21:
		AppendTextToEditCtrl(editError, L"���ʽ�ﲻ�ܳ��ֹ���\r\n");
		break;
	case 22:
		AppendTextToEditCtrl(editError, L"������ȱʧ\r\n");
		break;
	case 23:
		AppendTextToEditCtrl(editError, L"���Ӳ�����������ſ�ʼ\r\n");
		break;
	case 24:
		AppendTextToEditCtrl(editError, L"���ʽ������������ſ�ʼ\r\n");
		break;
	case 25:
		AppendTextToEditCtrl(editError, L"���ֹ���\r\n");
		break;
	case 26:
		AppendTextToEditCtrl(editError, L"�������������\r\n");
		break;
	case 27:
		AppendTextToEditCtrl(editError, L"repeat���ȱ��until\r\n");
		break;
	case 28:
		AppendTextToEditCtrl(editError, L"ȱ������\r\n");
		break;
	case 29:
		AppendTextToEditCtrl(editError, L"ȱ�� while\r\n");
		break;
	case 30:
		AppendTextToEditCtrl(editError, L"ȱ�� main\r\n");
		break;
	case 31:
		AppendTextToEditCtrl(editError, L"ʹ����������������ж���\r\n");
		break;
	default:
		AppendTextToEditCtrl(editError, L"δ֪����\r\n");
		break;
	}
}