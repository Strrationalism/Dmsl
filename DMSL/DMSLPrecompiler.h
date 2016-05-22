#pragma once
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <string>


namespace Dmsl {
	namespace Compiler {

		//预编译成果
		struct Precompiled {
			struct Unit {
				std::vector<std::string> initProgram, mainProgram;	//初始化程序，主程序代码
				std::map<std::string, uint32_t> varTable;	//私有变量链接表
				uint32_t memSize;	//内存占用大小
				uint32_t num;	//程序单元编号
			};
			std::map<std::string, uint32_t> attributeVars, uniformVars,cFuncs,cMet;	//属性变量，非易变变量链接表，C语言函数链接表，C语言方法
			std::map<std::string, float> constants;	//常量
			std::map<uint32_t, uint32_t> metParamNum, funcParmNum;	//方法参数数量和函数方法数量
			std::vector<Unit> units;	//程序单元
		};

		//预编译代码，返回true为成功
		bool Precompile(const std::string& codeStr,Precompiled&,std::ostream& log);

		//显示预编译后的代码
		void DisplayPrecompiled(const Precompiled&, std::ostream&);
	}
}
