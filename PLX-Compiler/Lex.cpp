#include "stdafx.h"
#include "PLX.h"
#include "PLX-Compiler.h"
#include "PLX-CompilerDlg.h"

lexelem lex_list[LEX_LIST_SIZE];
int lex_length = 0;

char nextChar(FILE *in_file_ptr) {
	//char c = fgetc(in_file_ptr);
	//printf("%c", c);
	return fgetc(in_file_ptr);
}

void lex(FILE *in_file_ptr) {
	int lex_index = 0;
	for (int i = 0; i < lex_length; i++)
		lex_list[i].token = illegal_sym;
	lex_length = 0;
	errorcnt = 0;

	char c;
	int ahead = 0; //往前读的字符数
	int comments = 0;

	c = nextChar(in_file_ptr);
	while (c != EOF) {
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
			c = nextChar(in_file_ptr);
			ahead = 0;
			continue;
		}

		//字母
		if (isalpha(c)) {
			char str_buf[IDENTIFIER_LENGTH + 5];
			memset(str_buf, 0, sizeof(str_buf));

			int i = 0;
			str_buf[i] = c;

			i++;
			ahead = 1;

			while (isalpha(c = nextChar(in_file_ptr)) || isdigit(c)) {
				if (i > IDENTIFIER_LENGTH) {
					while (isalpha(c = nextChar(in_file_ptr)) || isdigit(c));
					break;
				}
				str_buf[i++] = c;
			}

			//检查保留字
			/***********可以使用二分查找法*************/
			int is_reserver = -1;
			for (int i = 0; i < RESERVED_WORD_SIZE; i++)
				if (strcmp(str_buf, RESERVED_WORD[i]) == 0) {
					is_reserver = i;
					break;
				}

			switch (is_reserver) {
				case 0:
					lex_list[lex_index].token = main_sym;
					break;
				case 1:
					lex_list[lex_index].token = const_sym;
					break;
				case 2:
					lex_list[lex_index].token = var_sym;
					break;
				case 3:
					lex_list[lex_index].token = proc_sym;
					break;
				case 4:
					lex_list[lex_index].token = call_sym;
					break;
				//case 4:
					//lex_list[lex_index].token = begin_sym;
					//break;
				//case 5:
					//lex_list[lex_index].token = end_sym;
					//break;
				case 5:
					lex_list[lex_index].token = if_sym;
					break;
				case 6:
					lex_list[lex_index].token = then_sym;
					break;
				case 7:
					lex_list[lex_index].token = else_sym;
					break;
				case 8:
					lex_list[lex_index].token = while_sym;
					break;
				case 9:
					lex_list[lex_index].token = do_sym;
					break;
				case 10:
					lex_list[lex_index].token = read_sym;
					break;
				case 11:
					lex_list[lex_index].token = write_sym;
					break;
				/*case 13:
					lex_list[lex_index].token = odd_sym;*/
					break;
				case 12:
					lex_list[lex_index].token = repeat_sym;
					break;
				case 13:
					lex_list[lex_index].token = until_sym;
					break;
				case 14:
					lex_list[lex_index].token = break_sym;
					break;
				case 15:
					lex_list[lex_index].token = continue_sym;
					break;
				default:
					lex_list[lex_index].token = ident_sym;
					strcpy_s(lex_list[lex_index].name, str_buf);
					break;
			}
			lex_index += 1;
		}

		//数字
		else if (isdigit(c)) {
			int number = c - '0';
			ahead = 1;
			int length = 1;

			while (isdigit(c = nextChar(in_file_ptr))) {
				if (length > NUM_LENGTH) { // 超出数字位数长度， 将低位舍弃。如: 123456789->12345678
					while (isdigit(c = nextChar(in_file_ptr)));
					break;
				}
				number = 10 * number + (c - '0');
				length += 1;
			}

			lex_list[lex_index].token = number_sym;
			lex_list[lex_index].value = number;
			lex_index += 1;
		}

		//特殊符号
		else {
			ahead = 0;
			char special_sym = -1;
			for (int i = 0; i < SPECIAL_SYM_SIZE; i++)
				if (c == SPECIAL_SYMBOLS[i]) {
					special_sym = i;
					break;
				}

			switch (special_sym) {
			case 0:
				lex_list[lex_index].token = plus_sym;
				lex_index += 1;
				break;
			case 1:
				lex_list[lex_index].token = minus_sym;
				lex_index += 1;
				break;
			case 2:
				lex_list[lex_index].token = mult_sym;
				lex_index += 1;
				break;

				//除法或注释
			case 3:
				c = nextChar(in_file_ptr);
				ahead = 1;
				if (c == '*') {
					comments = 1;
					ahead = 0;
					c = nextChar(in_file_ptr);
					while (comments == 1) {
						if (c == '*') {
							c = nextChar(in_file_ptr);
							if (c == '/') comments = 0;
						}
						else c = nextChar(in_file_ptr);
					}
				}
				else {
					lex_list[lex_index].token = slash_sym;
					lex_index += 1;
				}
				break;

			case 4:
				lex_list[lex_index].token = lparent_sym;
				lex_index += 1;
				break;
			case 5:
				lex_list[lex_index].token = rparent_sym;
				lex_index += 1;
				break;
			case 6:
				c = nextChar(in_file_ptr);
				ahead = 1;
				if (c == '=') {
					lex_list[lex_index].token = eq_sym;
					ahead = 0;
				}
				else {
					lex_list[lex_index].token = becomes_sym;
				}
				lex_index += 1;
				break;
			case 7:
				lex_list[lex_index].token = comma_sym;
				lex_index += 1;
				break;
			case 8:
				c = nextChar(in_file_ptr);
				ahead = 1;
				if (c == '=') {
					lex_list[lex_index].token = neq_sym;
					ahead = 0;
				}
				else {
					lex_list[lex_index].token = not_sym;
				}
				lex_index += 1;
				break;
			case 9:
				c = nextChar(in_file_ptr);
				ahead = 1;
				if (c == '=') {
					lex_list[lex_index].token = leq_sym;
					ahead = 0;
				}
				else {
					lex_list[lex_index].token = les_sym;
				}
				lex_index += 1;
				break;
			case 10:
				c = nextChar(in_file_ptr);
				ahead = 1;
				if (c == '=') {
					lex_list[lex_index].token = geq_sym;
					ahead = 0;
				}
				else {
					lex_list[lex_index].token = gtr_sym;
				}
				lex_index += 1;
				break;
			case 11:
				lex_list[lex_index].token = semicolon_sym;
				lex_index += 1;
				break;
			case 12:
				lex_list[lex_index].token = mod_sym;
				lex_index += 1;
				break;
			case 13:
				lex_list[lex_index].token = lbrace_sym;
				lex_index += 1;
				break;
			case 14:
				lex_list[lex_index].token = rbrace_sym;
				lex_index += 1;
				break;
			case 15:
				lex_list[lex_index].token = lbracket_sym;
				lex_index += 1;
				break;
			case 16:
				lex_list[lex_index].token = rbracket_sym;
				lex_index += 1;
				break;
			default:
				editError->SetWindowTextW(CString("Invalid symbol\r\n"));
				errorcnt = 1;
				break;
			}
		}
		if (ahead == 0)
			c = nextChar(in_file_ptr);
	}
	lex_length = lex_index;
}