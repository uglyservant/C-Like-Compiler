#include "stdafx.h"
#include "PLX.h"
#include "PLX-Compiler.h"
#include "PLX-CompilerDlg.h"

void printStackFrame(int SP, int BP);
int executeCycle(aop * instru, int * sp, int * bp, int * pc, FILE * ifp);
int OPR(int * sp, int * bp, int * pc, aop * instru);
int base(int * bp, aop * instru);

int stack[STACK_SIZE];

CString tmp;

void interpret(FILE * ifp) {
	aop * instru;

	int SP = 0;
	int BP = 1;
	int PC = 0;

	int chx = 0;
	int ret;

	while (BP != 0) {
		//�����ǰָ��
		instru = &code[PC];

		CString serial, l_cstr, a_cstr;
		l_cstr.Format(L"%d", instru->l);
		a_cstr.Format(L"%d", instru->a);
		serial.Format(L"%d", PC);
		listCodeHist->InsertItem(chx, serial);
		listCodeHist->SetItemText(chx, 1, CString(PCODE_STR[instru->op]));
		listCodeHist->SetItemText(chx, 2, l_cstr);
		listCodeHist->SetItemText(chx, 3, a_cstr);

		PC += 1;

		//ִ��ָ��
		ret = executeCycle(instru, &SP, &BP, &PC, ifp);
		if (ret != 0) BP = 0;

		//���ջ֡
		tmp = L"";
		printStackFrame(SP, BP);
		listCodeHist->SetItemText(chx, 4, tmp);

		chx += 1;
	}
}

void printStackFrame(int SP, int BP) {
	CString str;
	//BPΪ0���������
	if (BP == 0) return;
	//��ջ֡
	else if (BP == 1) {
		for (int i = 1; i <= SP; i++) {
			str.Format(L"%d ", stack[i]);
			tmp += str;
		}
		return;
	}
	//���ڵ���
	else {
		printStackFrame(BP - 1, stack[BP + 1]);

		//Covers one case, where CAL instruction is just called, meaning a new Stack Frame is created, but SP is still less than BP
		if (SP < BP) {
			tmp += L"| ";
			for (int i = 0; i < 4; i++) {
				str.Format(L"%d ", stack[BP + i]);
				tmp += str;
			}
		}
		//For SP being greater than BP, aka most cases
		else {
			tmp += L"| ";
			for (int i = BP; i <= SP; i++) {
				str.Format(L"%d ", stack[i]);
				tmp += str;
			}
		}
		return;
	}
}

int executeCycle(aop * instru, int* sp, int* bp, int* pc, FILE * ifp) {
	CString outstr;
	switch (instru->op) {
	case lit_op: //LIT
		*sp = *sp + 1;
		if (*sp >= STACK_SIZE) {
			AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: �þ�ջ�ռ�\r\n");
			return -1;
		}
		stack[*sp] = instru->a;
		break;
	case opr_op: //OPR function
		return OPR(sp, bp, pc, instru);
		break;
	case lod_op: //LOD
		*sp = *sp + 1;
		if (*sp >= STACK_SIZE) {
			AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: �þ�ջ�ռ�\r\n");
			return -1;
		}
		stack[*sp] = stack[base(bp, instru) + instru->a];
		break;
	case sto_op: //STO
		stack[base(bp, instru) + instru->a] = stack[*sp];
		*sp = *sp - 1;
		break;
	case cal_op: //CAL
		//if (*sp + 4 >= STACK_SIZE) {
		//	AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: �þ�ջ�ռ�\r\n");
		//	return -1;
		//}
		//stack[*sp + 1] = 0; //RET
		//stack[*sp + 2] = base(bp, instru); //SL
		//stack[*sp + 3] = *bp; //DL
		//stack[*sp + 4] = *pc; //RA
		//*bp = *sp + 1;
		//*pc = instru->a;
		//break;
		if (*sp + 3 >= STACK_SIZE) {
			AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: �þ�ջ�ռ�\r\n");
			return -1;
		}
		stack[*sp + 1] = base(bp, instru); //SL
		stack[*sp + 2] = *bp; //DL
		stack[*sp + 3] = *pc; //RA
		*bp = *sp + 1;
		*pc = instru->a;
		break;
	case int_op: //INT
		*sp = *sp + instru->a;
		if (*sp >= STACK_SIZE) {
			AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: �þ�ջ�ռ�\r\n");
			return -1;
		}
		break;
	case jmp_op: //JMP
		*pc = instru->a;
		break;
	case jpc_op: //JPC
		if (stack[*sp] == 0)
			*pc = instru->a;
		*sp = *sp - 1;
		break;
	case red_op: //RED
		*sp = *sp + 1;
		if (*sp >= STACK_SIZE) {
			AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: �þ�ջ�ռ�\r\n");
			return -1;
		}
		if (fscanf_s(ifp, "%d", &stack[*sp]) == EOF) {
			AppendTextToEditCtrl(editOutput, L"������������в���!\r\n");
			return -1;
		}
		break;
	case wrt_op: //WRT
		outstr.Format(L"%d\r\n", stack[*sp]); //������
		AppendTextToEditCtrl(editOutput, outstr);
		*sp = *sp - 1;
		break;
	case sta_op: //STO TO ARRAY
		stack[base(bp, instru) + instru->a + stack[*sp - 1]] = stack[*sp];
		*sp = *sp - 2;
		break;
	case lda_op:
		//*sp = *sp;
		if (*sp + 1 >= STACK_SIZE) {
			AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: �þ�ջ�ռ�\r\n");
			return -1;
		}
		stack[*sp] = stack[base(bp, instru) + instru->a + stack[*sp]];
		break;
	default:
		AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: Illegal OPR!\r\n");
		return -1;
		break;
	}
	return 0;
}

int OPR(int *sp, int* bp, int *pc, aop * instru) {
	switch (instru->a) {
	case ret_op:
		*sp = *bp - 1;
		*bp = stack[*sp + 2];
		*pc = stack[*sp + 3];
		break;
	case neg_op:
		stack[*sp] = -stack[*sp];
		break;
	case add_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] + stack[*sp + 1];
		break;
	case sub_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] - stack[*sp + 1];
		break;
	case mul_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] * stack[*sp + 1];
		break;
	case div_op:
		*sp = *sp - 1;
		if (stack[*sp + 1] == 0) {
			AppendTextToEditCtrl(editOutput, L"RUNTIME ERROR: �������\r\n");
			return -1;
		}
		stack[*sp] = stack[*sp] / stack[*sp + 1];
		break;
	case not_op:
		if (stack[*sp] != 0)
			stack[*sp] = 0;
		else
			stack[*sp] = 1;
		break;
	case mod_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] % stack[*sp + 1];
		break;
	case eql_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] == stack[*sp + 1];
		break;
	case neq_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] != stack[*sp + 1];
		break;
	case lss_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] < stack[*sp + 1];
		break;
	case leq_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] <= stack[*sp + 1];
		break;
	case gtr_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] > stack[*sp + 1];
		break;
	case geq_op:
		*sp = *sp - 1;
		stack[*sp] = stack[*sp] >= stack[*sp + 1];
		break;
	}
	return 0;
}

int base(int * bp, aop * instru) {
	int l = instru->l;
	int b1;
	b1 = *bp;
	while (l > 0) {
		b1 = stack[b1];
		l--;
	}
	return b1;
}
