#include "DMSL.h"
#include "DMSLElf.h"
#include <iostream>
#include <array>
#include "DmslCompiler.h"
//#define _OUT_ASM

#pragma warning(disable:4244)
#pragma warning(disable:4267)

#define DVASSERT(x) if(!(x)) Crash(L"DmslVM Crashed.")

using namespace std;
using namespace Dmsl;

std::stack<DmslUnit*> DmslUnit::m_nowRunning;

DmslVirtualMachine::DmslVirtualMachine(const std::string& programCode,std::ostream& log){
	Dmsl::Compiler::Precompiled p;
	bool b1 = Dmsl::Compiler::Precompile(programCode, p,log);
	bool b2 = Dmsl::Compiler::Compile(p,m_program,log);

	//Alloc Memory
	m_cMetLinks.resize(m_program.linkMetSize);
	m_cFuncLinks.resize(m_program.linkFuncSize, nullptr);
   // m_uniform.resize(m_program.uniformSize);

	m_good = b1&&b2;
}

static std::string getString(const char*& p) {
	std::string ret;
	uint32_t size = *((uint32_t*)p); p += sizeof(size);
	
	for (uint32_t i = 0; i < size; ++i) 
		ret += ~(*(p++));
	return ret;
}

static void getBinary(const char*& p, std::vector<uint8_t>& v) {
	uint32_t size = *((uint32_t*)p); p += sizeof(size);

	v.resize(size);
	memcpy(&v[0], p, size);
	p += size;
}

Dmsl::DmslVirtualMachine::DmslVirtualMachine(const char * program)
{
	//这里读入二进制文件
	m_program.linkMetSize = *(uint32_t*)program;
	program += sizeof(uint32_t);
	m_program.linkFuncSize = *(uint32_t*)program;
	program += sizeof(uint32_t);

	uint16_t size = *((uint16_t*)program); program += sizeof(uint16_t);
	for (uint16_t i = 0; i < size; ++i) {
		auto first = getString(program);
		uint32_t second = *((uint32_t*)program); program += sizeof(uint32_t);
		m_program.custumProgramHandles[first] = second;
	}


	m_program.linkTable.clear();
	size = *((uint16_t*)program); program += sizeof(uint16_t);
	for (uint16_t i = 0; i < size;++i) {
		auto first = getString(program);
		uint32_t second = *((uint32_t*)program); program += sizeof(second);
		m_program.linkTable[first] = second;
	}

	//m_program.uniformSize = *((uint32_t*)program); program += sizeof(m_program.uniformSize);

	m_program.units.clear();
	size = *((uint16_t*)program); program += sizeof(uint16_t);
	for (uint16_t i = 0; i < size; ++i) {
		auto& unit = m_program.units[*((uint32_t*)program)];
		program += sizeof(uint32_t);

		unit.outVaryingSize = *((uint32_t*)program); program += sizeof(uint32_t);
		unit.memSize = *((uint32_t*)program); program += sizeof(uint32_t);

		getBinary(program, unit.initProgram);
		getBinary(program, unit.mainProgram);

		uint16_t sizeIn = *((uint16_t*)program); program += sizeof(uint16_t);
		for (uint16_t j = 0; j < sizeIn; ++j){
			auto& bin = unit.custumProgram[*((uint32_t*)program)];
			program += sizeof(uint32_t);
			getBinary(program, bin);
		}

		sizeIn = *((uint16_t*)program); program += sizeof(uint16_t);
		for (uint16_t j = 0; j < sizeIn; ++j) {
			unit.eventProgram.push_back({ *((uint32_t*)program),std::vector<uint8_t>() });
			auto& bin = unit.eventProgram.back().second;
			program += sizeof(uint32_t);
			getBinary(program, bin);
		}
	}

	//分配内存
	m_cMetLinks.resize(m_program.linkMetSize);
	m_cFuncLinks.resize(m_program.linkFuncSize, nullptr);
	//m_uniform.resize(m_program.uniformSize);
	m_good = true;
}
/*
void DmslVirtualMachine::SetUniform(const std::string& uniform,double f){
    m_uniform[m_program.linkTable.at(uniform)] = f;
}*/

void DmslVirtualMachine::LinkCFunc(const std::string& cfunc,double(*f)(double*,void*)){
    m_cFuncLinks[m_program.linkTable.at(cfunc)] = f;
}

void DmslVirtualMachine::LinkCMet(const std::string& cmet,void(*f)(double*, void*)){
    m_cMetLinks[m_program.linkTable.at(cmet)] = f;
}

void Dmsl::DmslVirtualMachine::Ready()
{
	for (auto& unit : m_program.units) {
		readyCode(unit.second.initProgram);
		readyCode(unit.second.mainProgram);
		for (auto& customMethod : unit.second.custumProgram)
			readyCode(customMethod.second);
		for (auto& p : unit.second.eventProgram)
			readyCode(p.second);
	}

	m_cFuncLinks.clear();
	m_cMetLinks.clear();
}

void DmslUnit::SetAttribute(const std::string& attribute,double f){
    m_attribute[m_dvm->m_program.linkTable.at(attribute)] = f;
}

double DmslUnit::GetAttribute(const std::string& attribute) const
{
    return m_attribute[m_dvm->m_program.linkTable.at(attribute)];
}

Dmsl::DmslUnit::DmslUnit(const DmslVirtualMachine& dvm,uint32_t unit, std::shared_ptr<const std::vector<double>> varyingIn){
    if(!dvm.Good()) throw runtime_error("Dmsl虚拟机中的代码不正确！");
    if(!dvm.HaveUnit(unit)) throw runtime_error("没有在Dmsl虚拟机中找到对应程序单元 " + to_string(unit)+"！");
    m_attribute.resize(dvm.m_program.units.at(unit).memSize);
    m_dvm = &dvm;
    m_unit = &(m_dvm->m_program.units.at(unit));
	m_varying = std::make_shared<std::vector<double>>();
	m_varying->resize(m_unit->outVaryingSize);
	m_varyingIn = varyingIn;
	m_pausePoint = nullptr;
}

static double inline GetDouble(const uint8_t*& address){
    #ifdef _OUT_ASM
    cout<<"  "<< *(double*)(address);
    #endif
	address += sizeof(double);
	return *(double*)(address - sizeof(double));
};

static uintptr_t inline GetAddress(const uint8_t*& address){
    #ifdef _OUT_ASM
    cout<<"  (ADDRESS)"<< (uint64_t)(*(uintptr_t*)(address));
    #endif
	address += sizeof(uintptr_t);
	return *(uintptr_t*)(address - sizeof(uintptr_t));
};

#define PUSH_FLOAT(x) stk.at(stkSize++).f = (x);
#define PUSH_ADDRESS(x) stk.at(stkSize++).a = (x);

#define POP_FLOAT() stk.at(--stkSize).f;
#define POP_ADDRESS() stk.at(--stkSize).a;

void Dmsl::DmslVirtualMachine::readyCode(std::vector<uint8_t>& code)
{
	enum ArgType{ADDRESS,DMOVFS,VAL,ADDRESS_JMP_LINK,ADDRESS_CMET_LINK,ADDRESS_CFUNC_LINK};
	const std::vector<ArgType> oprArgList[0x1F] = {
		{ DMOVFS },	//DMOVFS
		{ ADDRESS },	//MOV
		{ ADDRESS_CFUNC_LINK,ADDRESS },	//CFUNC
		{ ADDRESS_CMET_LINK,ADDRESS },	//CMET
		{ VAL },	//PUSH
		{ ADDRESS_JMP_LINK },	//IF
		{ ADDRESS_JMP_LINK },	//JMP
		{},	//END
		{ ADDRESS },	//DUNPA
		{ ADDRESS },	//DUNPU
		{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},
		{ ADDRESS },	//VARYING_MOV
		{ ADDRESS },
		{ ADDRESS },
		{  },	//基于时间轴的暂停
		{ADDRESS},
		{ADDRESS}
	};

	for (uintptr_t i = 0; i < code.size();) {
		auto opr = (Dmsl::VirtualMachine::ElfCode)code[i++];
		if (!oprArgList[(int)opr].empty()) {
			//收集参数
			if (oprArgList[(int)opr][0] == DMOVFS) {
				//if (i + 1 >= code.size()) continue;
				//跳过DMOVFS参数
				i += sizeof(uintptr_t);
				auto num = (uintptr_t&)code[i];
				i += sizeof(uintptr_t);
				i += sizeof(double) * num;
				
			}
			else {
				//跳过其它参数
				for (auto type : oprArgList[int(opr)]) {
					if (type == ADDRESS) {
						i += sizeof(uintptr_t);
					}
					else if (type == VAL) {
						i += sizeof(double);
					}
					else if (type == ADDRESS_JMP_LINK) {
						(uintptr_t&)code[i] = uintptr_t((uintptr_t&)code[i] + &code[0]);
						i += sizeof(uintptr_t);
					}
					else if (type == ADDRESS_CMET_LINK) {
						(uintptr_t&)code[i] = uintptr_t(m_cMetLinks[(uintptr_t&)code[i]]);
						i += sizeof(uintptr_t);
					}
					else if (type == ADDRESS_CFUNC_LINK) {
						(uintptr_t&)code[i] = uintptr_t(m_cFuncLinks[(uintptr_t&)code[i]]);
						i += sizeof(uintptr_t);
					}
				}
			}
		}
	}
}
void DmslVirtualMachine::runCode(const uint8_t*& pausePoint, DmslUnit* unitObj,const std::vector<uint8_t>& code,std::vector<double>* attrMem, std::vector<double>* varying, const std::vector<double>* varyingIn, void* userdata) const{
	const uint8_t *address = pausePoint ? pausePoint : &code[0];
	pausePoint = nullptr;

    static union data{double f;uintptr_t a;}d;
	static std::array<data,4096> stk;
	int stkSize = 0;
	using Dmsl::VirtualMachine::ElfCode;

    while(1){
        
        #ifdef _OUT_ASM
        cout<<endl<<(int)*address;
        #endif

		//崩溃时把这个代码打开
		//if (stkSize >= 16) Crash(L"DmslVM栈溢出。");

        switch((Dmsl::VirtualMachine::ElfCode)*address++){
        case ElfCode::DMOVFS:{
				uintptr_t varAddress = GetAddress(address);
                auto srcCount = GetAddress(address);
                for(int i = 0;i < m_dmovfs_sel;++i) GetDouble(address);
                attrMem ->at(varAddress) = GetDouble(address);
                for(uint32_t i = 0;i < srcCount - m_dmovfs_sel - 1;++i) GetDouble(address);
            }
            break;
        case ElfCode::MOV:
			if (stkSize >= 1) {
				attrMem->at(GetAddress(address)) = POP_FLOAT();
			}
			else {
				throw std::exception("ElfVMCrashed:Stack overflow.");
			}
            break;
        case ElfCode::CFUNC:{
                auto func = (double(*)(double*, void*))GetAddress(address);
				auto paramSize = GetAddress(address);

				auto ret = (func((double*)&stk.at(stkSize - paramSize), userdata));
				stkSize = stkSize - paramSize + 1;
				stk.at(stkSize - 1).f = ret;

            }
            break;
        case ElfCode::CMET:{
				auto func = (void(*)(double*, void*))GetAddress(address);
				auto paramSize = GetAddress(address);
				
				func((double*)&stk.at(stkSize - paramSize), userdata);
				stkSize -= paramSize;
            }
            break;
        case ElfCode::PUSH:
            PUSH_FLOAT(GetDouble(address));
            break;
        case ElfCode::IF:
            if(!stk.at(stkSize-1).a) address = (const uint8_t*)GetAddress(address);
            else GetAddress(address);
			stkSize--;
            break;
        case ElfCode::JMP:
            address = (const uint8_t*)GetAddress(address);
            break;
        case ElfCode::END:
            return;
            break;
        case ElfCode::DUNPA:
            PUSH_FLOAT(attrMem ->at(GetAddress(address)));
            break;
        /*case ElfCode::DUNPU:
            PUSH_FLOAT(m_uniform[GetAddress(address)]);
            break;*/
        case ElfCode::NEGA:
			if (stkSize >= 1)
				stk.at(stkSize - 1).f = -stk.at(stkSize - 1).f;
			else
				throw std::exception("ElfVMCrashed:Stack overflow.");
            break;
        case ElfCode::ADD:
			stk.at(stkSize - 2).f += stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::SUB:
			stk.at(stkSize - 2).f -= stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::MUL:
			stk.at(stkSize - 2).f *= stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::DIV:
			stk.at(stkSize - 2).f /= stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::MOD:
			stk.at(stkSize - 2).f = (int)stk.at(stkSize - 2).f % (int)stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::BIG:
			stk.at(stkSize - 2).a = stk.at(stkSize - 2).f > stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::BIGE:
			stk.at(stkSize - 2).a = stk.at(stkSize - 2).f >= stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::SML:
			stk.at(stkSize - 2).a = stk.at(stkSize - 2).f < stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::SMLE:
			stk.at(stkSize - 2).a = stk.at(stkSize - 2).f <= stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::EQ:
			stk.at(stkSize - 2).a = stk.at(stkSize - 2).f == stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::NE:
			stk.at(stkSize - 2).a = stk.at(stkSize - 2).f != stk.at(stkSize-1).f;
			--stkSize;
            
            break;
        case ElfCode::AND:
			stk.at(stkSize - 2).a = stk.at(stkSize - 2).a && stk.at(stkSize-1).a;
			--stkSize;
            
            break;
        case ElfCode::OR:
			stk.at(stkSize - 2).a = stk.at(stkSize - 2).a || stk.at(stkSize-1).a;
			--stkSize;
            
            break;
        case ElfCode::NOT:
			stk.at(stkSize-1).a = !stk.at(stkSize-1).a;
            
            break;

		case ElfCode::VARYING_MOV: {
			auto varAddress = GetAddress(address);
			//DVASSERT(varAddress < varying->size());
			varying->at(varAddress) = stk.at(--stkSize).f;
			break;
		}
		case ElfCode::VARYING_READ_IN: {
			auto varAddress = GetAddress(address);
			//DVASSERT(varAddress < varyingIn->size());
			stk.at(stkSize++).f = varyingIn->at(varAddress);
			break;
		}
		case ElfCode::VARYING_UNP: {
			auto varAddress = GetAddress(address);
			//DVASSERT(varAddress < varying->size());
			stk.at(stkSize++).f = varying->at(varAddress);
			break;
		}
		case ElfCode::PAUSE: {
			uint32_t s = (uint32_t)stk.at(--stkSize).f;
			if(s) unitObj->Sleep(s);
			pausePoint = address;
			return;
		}
		case ElfCode::DATAACTOR_ADD_ATTR: {
			auto end = stk.at(stkSize - 3).f;
			auto time = stk.at(stkSize - 2).f;
			auto func = stk.at(stkSize - 1).f;
			unitObj->GetDataActor().Add(attrMem->at(GetAddress(address)), end, time, func);
			stkSize -= 3;
			break;
		}
		case ElfCode::DATAACTOR_ADD_VARYING: {
			auto end = stk.at(stkSize - 3).f;
			auto time = stk.at(stkSize - 2).f;
			auto func = stk.at(stkSize - 1).f;
			unitObj->GetDataActor().Add(varying->at(GetAddress(address)), end, time, func);
			stkSize -= 3;
			break;
		}
        };
    }
}
