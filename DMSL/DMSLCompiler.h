#pragma once
#include <map>
#include <vector>
#include <string>
#include <stack>
#include "DMSLPrecompiler.h"
#include "DMSLElf.h"

namespace Dmsl {
	namespace Compiler {
		struct Compiled {
			struct Unit {
				uint32_t memSize;
				std::vector<uint32_t> initProgram, mainProgram;
			};
			std::map<std::string, uint32_t> linkTable;
			std::map<uint32_t, Unit> units;
		};
		
		//编译
		void Compile(const Precompiled& in, Compiled& out);

		//把float压进代码
		void PushFloat(float, std::vector<uint32_t>& out);

		//把地址压入代码
		void PushAddress(uint32_t, std::vector<uint32_t>& out);

		//把指令压入代码
		void PushCmd(Dmsl::VirtualMachine::ElfCode, std::vector<uint32_t>& out);

		//解析出的表达式结构体
		struct ParsedMathWord {
			enum {
				NUMBER, VARNAME, OPR, FUNC_NAME
			}type;
			bool nega = false;	//相反数
			union {
				std::string varName;
				std::string funcName;
				float number;
				struct Opr{
					int level;	//优先级记录
					enum {
						COMMA,	//1级优先级
						AND,
						OR,
						NOT,	//2级优先级
						NE,	//3级优先级
						EQ,
						BIG,
						BIGE,
						SML,
						SMLE,
						ADD,	//4级
						SUB,
						MUL,	//5
						DIV,
						MOD,
						CALL_FUNC	//6
					}opr;	//操作符
					//遇见左括号，优先级+10，右括号-10
				}opr;
			};
		};

		//解析表达式到代码
		void ParseMath(const Precompiled& in, std::string, std::vector<uint32_t>& out);

		//解析表达式下层函数 -- 压入扫描到的数据
		void ParseData(const Precompiled& in, const Precompiled::Unit& unit,const ParsedMathWord&, std::vector<uint32_t>& out);

		//解析表达式下层函数 -- 压入扫描到的操作符
		void ParseOperator(const Precompiled& in,const ParsedMathWord&, std::vector<uint32_t>& out,std::stack<ParsedMathWord::Opr>& stk);

		//查询物体类型
		//0 - 不知道
		//1 - attribute
		//2 - unit::var
		//3 - uniform
		//4 - constant_define
		//5 - number
		//6 - cfunc
		//7 - cmethod
		int WhatsTheVar(const Precompiled& in,const Precompiled::Unit& unit, const std::string& name);

	}
}