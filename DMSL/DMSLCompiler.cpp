#include "DMSLCompiler.h"
#include "DMSLUtils.h"
#include <stdexcept>
using namespace std;
using namespace Dmsl::Utils;

void Dmsl::Compiler::Compile(const Precompiled & in, Compiled & out)
{
}

void Dmsl::Compiler::PushFloat(float a, std::vector<uint32_t>& out)
{
	for (uint16_t i = 0; i < sizeof(float); ++i) {
		uint8_t* c = (uint8_t*)&a;
		out.push_back(c[i]);
	}
}

void Dmsl::Compiler::PushAddress(uint32_t a, std::vector<uint32_t>& out)
{
	for (uint16_t i = 0; i < sizeof(uint32_t); ++i) {
		uint8_t* p = (uint8_t*)&a;
		out.push_back(p[i]);
	}
}

void Dmsl::Compiler::PushCmd(Dmsl::VirtualMachine::ElfCode c, std::vector<uint32_t>& out)
{
	out.push_back((uint8_t)c);
}

void Dmsl::Compiler::ParseMath(const Precompiled & in, std::string  s, std::vector<uint32_t>& out)
{
	s = TrimAll(s);
	if (s.empty()) throw runtime_error("无效的表达式。");

	
}

void Dmsl::Compiler::ParseData(const Precompiled & in,const Precompiled::Unit& unit,const ParsedMathWord & s, std::vector<uint32_t>& out)
{
	if (s.type == ParsedMathWord::NUMBER) {
		PushCmd(VirtualMachine::ElfCode::PUSH, out);
		PushFloat(s.number, out);
	}
	else if (s.type == ParsedMathWord::VARNAME) {
		switch (WhatsTheVar(in, unit, s.varName)) {
		case 1:
			PushCmd(VirtualMachine::ElfCode::PUSHPTR, out);
			PushAddress(in.attributeVars.at(s.varName), out);
			PushCmd(VirtualMachine::ElfCode::UNPA, out);
			break;
		case 2:
			PushCmd(VirtualMachine::ElfCode::PUSHPTR, out);
			PushAddress(unit.varTable.at(s.varName), out);
			PushCmd(VirtualMachine::ElfCode::UNPA, out);
			break;
		case 3:
			PushCmd(VirtualMachine::ElfCode::PUSHPTR, out);
			PushAddress(in.uniformVars.at(s.varName), out);
			PushCmd(VirtualMachine::ElfCode::UNPU, out);
			break;
		case 4:
			PushCmd(VirtualMachine::ElfCode::PUSH, out);
			PushFloat(in.constants.at(s.varName), out);
			break;
		case 5:
			PushCmd(VirtualMachine::ElfCode::PUSH, out);
			PushFloat(s.number, out);
			break;
		default:
			throw runtime_error("虽然不知道发生了什么，但是这里表达式不正常。");
		}
	}
}

void Dmsl::Compiler::ParseOperator(const Precompiled & in, const ParsedMathWord & word, std::vector<uint32_t>& out, std::stack<ParsedMathWord::Opr>& stk)
{
	if (stk.empty()) {
		stk.push(word.opr);
	}
	else if (stk.top().level <= word.opr.level) {
		stk.push(word.opr);
	}
	else {
		while (stk.top().level > word.opr.level) {
			switch (stk.top().opr) {
			case ParsedMathWord::Opr::AND:
				PushCmd(VirtualMachine::ElfCode::AND, out); break;
			case ParsedMathWord::Opr::OR:
				PushCmd(VirtualMachine::ElfCode::OR, out); break;
			case ParsedMathWord::Opr::NOT:
				PushCmd(VirtualMachine::ElfCode::NOT, out); break;

			case ParsedMathWord::Opr::NE:
				PushCmd(VirtualMachine::ElfCode::NE, out); break;
			case ParsedMathWord::Opr::EQ:
				PushCmd(VirtualMachine::ElfCode::EQ, out); break;
			case ParsedMathWord::Opr::BIG:
				PushCmd(VirtualMachine::ElfCode::BIG, out); break;
			case ParsedMathWord::Opr::BIGE:
				PushCmd(VirtualMachine::ElfCode::BIGE, out); break;
			case ParsedMathWord::Opr::SML:
				PushCmd(VirtualMachine::ElfCode::SML, out); break;
			case ParsedMathWord::Opr::SMLE:
				PushCmd(VirtualMachine::ElfCode::SMLE, out); break;

			case ParsedMathWord::Opr::ADD:
				PushCmd(VirtualMachine::ElfCode::ADD, out); break;
			case ParsedMathWord::Opr::SUB:
				PushCmd(VirtualMachine::ElfCode::SUB, out); break;
			case ParsedMathWord::Opr::MUL:
				PushCmd(VirtualMachine::ElfCode::MUL, out); break;
			case ParsedMathWord::Opr::DIV:
				PushCmd(VirtualMachine::ElfCode::DIV, out); break;
			case ParsedMathWord::Opr::MOD:
				PushCmd(VirtualMachine::ElfCode::MOD, out); break;

			case ParsedMathWord::Opr::CALL_FUNC:
				PushCmd(VirtualMachine::ElfCode::CFUNC, out); break;
			case ParsedMathWord::Opr::COMMA:
				break;
			}
			stk.pop();
			if (stk.empty()) break;
		}
		stk.push(word.opr);
	}
}

int Dmsl::Compiler::WhatsTheVar(const Precompiled & in, const Precompiled::Unit& unit, const std::string & name)
{
	if (in.attributeVars.count(name)) return 1;
	else if (in.cFuncs.count(name)) return 6;
	else if (in.cMet.count(name)) return 7;
	else if (in.constants.count(name)) return 4;
	else if (in.uniformVars.count(name)) return 3;
	else if (unit.varTable.count(name)) return 2;
	else {
		bool isNum = true;
		for (char ch : name)
			if (!isdigit(ch) || ch != '.') isNum = false;
		if (isNum) return 5;
	}
	return 0;
}
