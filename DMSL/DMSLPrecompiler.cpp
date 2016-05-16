#include "DMSLPrecompiler.h"
#include "DMSLUtils.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
using namespace std;
using namespace Dmsl::Utils;

namespace Dmsl {
	namespace Compiler {
		//预编译代码
		void Precompile(const std::string& codeStr, Precompiled& comp,ostream& log) {
			int inProgram = 0;
			int nowLine = 0;
			stringstream code(codeStr);

			uint32_t layer = 0;	//当前代码块层数
			int32_t unitNum = -1;	//当前激活的程序单元

			uint32_t nextUniform = 0;	//下一个uniform的编号
			uint32_t nextAttribute = 0;	//下一个attribute的编号
			bool disableAttribute = false;	//禁止attribute出现
			uint32_t nextCFunc = 0;	//下一个C语言链接编号
			uint32_t nextCMet = 0;	//下一个C语言方法编号

			uint32_t nextVar = 0;	//下一个局部变量编号
			std::vector<Precompiled::Unit>::iterator nowUnit;	//当前的Unit
			enum { INIT, MAIN }nowProgram;	//当前程序块

			while (!code.eof()) {
				try {
					string line;
					getline(code, line);
					++nowLine;

					line = TrimRight(DeleteComments(line));
					if (line.empty()) continue;


					if (layer == 0) {	//全局层
						string left = ReadLeftWord(line);
						string right = ReadRightLine(line);
						if (left == "unit") {
							disableAttribute = true;
							layer++;
							unitNum = ToInt(TrimAll(right));
							comp.units.push_back(Precompiled::Unit());
							nowUnit = comp.units.end();
							--nowUnit;
							nowUnit->num = unitNum;
							nextVar = nextAttribute;
						}
						else if (left == "uniform") {
							vector<string> unis;
							Split(right, unis, ',');
							for (auto& s : unis) {
								s = TrimAll(s);
								CheckName(s);
								comp.uniformVars[s] = nextUniform++;
							}
						}
						else if (left == "attribute") {
							if (disableAttribute) throw runtime_error("此处不应再出现attribute，你应该把它放在最前边。");
							vector<string> attrs;
							Split(right, attrs, ',');
							for (auto& s : attrs) {
								s = TrimAll(s);
								CheckName(s);
								comp.attributeVars[s] = nextAttribute++;
							}
						}
						else if(left == "cfunc"){
							auto pos = right.find(',');
							if (pos == string::npos) throw runtime_error("错误的C语言函数链接：没提供参数个数？");
							auto name = right.substr(0, pos);
							name = TrimAll(name);
							CheckName(name);
							comp.cFuncs[name] = nextCFunc;
							comp.funcParmNum[nextCFunc++] = ToInt(TrimAll(right.substr(pos + 1, right.length() - pos - 1)));
						}
						else if (left == "cmethod") {
							auto pos = right.find(',');
							if (pos == string::npos) throw runtime_error("错误的C语言方法链接：没提供参数个数？");
							auto name = right.substr(0, pos);
							name = TrimAll(name);
							CheckName(name);
							comp.cMet[name] = nextCMet;
							comp.metParamNum[nextCMet++] = ToInt(TrimAll(right.substr(pos + 1, right.length() - pos - 1)));
						}
						else if (left == "const") {
							auto pos = right.find('=');
							auto name = right.substr(0, pos);
							name = TrimAll(name);
							CheckName(name);
							comp.constants[name] = ToFloat(TrimAll(right.substr(pos + 1, right.length() - pos - 1)));
						}
						else throw runtime_error("未知意图。");

					}
					else if (layer == 1) {	//在unit内
						string left = ReadLeftWord(line);
						string right = ReadRightLine(line);
						if (left == "end") {
							--layer;
							nowUnit -> memSize = nowUnit->varTable.size() + comp.attributeVars.size();
						}
						else if (left == "init") {
							++layer;
							nowProgram = INIT;
						}
						else if (left == "main") {
							++layer;
							nowProgram = MAIN;
						}
						else if (left == "var") {
							vector<string> vars;
							Split(right, vars, ',');
							for (auto& s : vars) {
								s = TrimAll(s);
								CheckName(s);
								nowUnit->varTable[s] = nextVar++;
							}
						}
						else throw runtime_error("未知意图。");
					}
					else if (layer >= 2) {	//在程序块内
						string left = ReadLeftWord(line);
						if (left == "end") --layer;
						else if (
							left == "if" ||
							left == "while" ||
							left == "for" ||
							left == "do"
							)
							++layer;
						if (nowProgram == INIT) nowUnit->initProgram.push_back(line);
						else if (nowProgram == MAIN) nowUnit->mainProgram.push_back(line);
					}
				}
				catch (exception& e) {
					log << "编译错误：在行" + to_string(nowLine) + "有" + e.what() << endl;
				}
				
			}
			if (layer != 0) log << "编译错误：存在没配对的代码块首尾！" << endl;
		}
		void DisplayPrecompiled(const Precompiled & comp, std::ostream & out)
		{
			out << "--Constants" << endl;
			for (auto& i : comp.constants) {
				out << i.first << " = " << i.second << endl;
			}

			out << "--Uniforms" << endl;
			for (auto& i : comp.uniformVars) {
				out << i.first << " address:" << i.second << endl;
			}

			out << "--Attributes" << endl;
			for (auto& i : comp.attributeVars) {
				out << i.first << " address:" << i.second << endl;
			}

			out << "--CFunctions" << endl;
			for (auto& i : comp.cFuncs) {
				out << i.first << " params:" << comp.funcParmNum.at(i.second) << " address:" << i.second << endl;
			}

			out << "--CMethods" << endl;
			for (auto& i : comp.cMet) {
				out << i.first << " params:" << comp.metParamNum.at(i.second) << " address:" << i.second << endl;
			}

			out << "--Units" << endl;
			for (auto& unit : comp.units) {
				out << "Number:" << unit.num << "  Mem Size:" << unit.memSize << endl;
				out << "  --Vars" << endl;
				for (auto& v: unit.varTable) {
					out << "       " << v.first << " address:" << v.second << endl;
				}
				out << "--InitProgram" << endl;
				for (auto& c : unit.initProgram) {
					out << "            " << c << endl;
				}
				out << "--MainProgram" << endl;
				for (auto& c : unit.mainProgram) {
					out << "            " << c << endl;
				}
			}

		}
	}
}