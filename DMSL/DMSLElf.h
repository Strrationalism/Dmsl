#pragma once
namespace Dmsl {
	namespace VirtualMachine {
		enum  class ElfCode {
			//控制指令
			DMOVFS = 0x10,	//多重赋值，后跟参数：uint32_t n,k;float datas[n];将虚拟机指定的一个float赋值到地址为k的变量上
			MOV = 0x11,	//从栈顶取值，赋值给参数（指针）指向的变量
			CFUNC = 0x12,	//调用函数，参数为函数地址，注意有返回值，栈表现不同
			CMET = 0x13,	//调用方法，参数为方法地址，没有返回值，栈表现不同
			UNPA = 0x14,	//解引用栈顶指针到attribute变量
			UNPU = 0x15,	//解引用栈顶指针到uniform变量
			PUSH = 0x16,	//压栈，float
			PUSHPTR = 0x17,	//压栈，uint32_t
			IF = 0x19,	//如果栈顶为bool = true，则无视此指令，否则跳转到指令参数指明的代码位置
			JMP = 0x1A,	//无条件跳转代码到参数位置
			END = 0xFF,	//结束执行

			//运算指令
			NEGA = 0x20,	//栈顶取反压栈

			//要求栈顶两个数为float，且压回float
			ADD = 0x21,	//加法
			SUB = 0x22,	//减法
			MUL = 0x23,	//乘法
			DIV = 0x24,	//除法
			MOD = 0x25,	//取整后求余

			//要求栈顶两个数为float，且压回BOOL
			BIG = 0x30,	//大于
			BIGE = 0x31,	//大于等于
			SML = 0x32,	//小于
			SMLE = 0x33,	//小于等于
			EQ = 0x34,	//等于
			NE = 0x35,	//不等于

			//要求栈顶两个数为BOOL，且压回BOOL
			AND = 0x40,	//与
			OR = 0x41,	//或
			NOT = 0x42	//非
		};
	}
}