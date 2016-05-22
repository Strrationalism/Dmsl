#include "DMSL.h"
#include "DMSLElf.h"
#include <iostream>
#include "DmslCompiler.h"
#define _OUT_ASM

using namespace std;
using namespace Dmsl;

DmslVirtualMachine::DmslVirtualMachine(const std::string& programCode,std::ostream& log){
	Dmsl::Compiler::Precompiled p;
	bool b1 = Dmsl::Compiler::Precompile(programCode, p,log);
	bool b2 = Dmsl::Compiler::Compile(p,m_program,log);

	//Alloc Memory
	m_cMetLinks.resize(m_program.cMetSize);
	m_cFuncLinks.resize(m_program.cFuncSize);
    m_uniform.resize(m_program.uniformSize);

	m_good = b1&&b2;
}

void DmslVirtualMachine::SetUniform(const std::string& uniform,float f){
    m_uniform[m_program.linkTable.at(uniform)] = f;
}

void DmslVirtualMachine::LinkCFunc(const std::string& cfunc,float(*f)(float*)){
    m_cFuncLinks[m_program.linkTable.at(cfunc)] = f;
}

void DmslVirtualMachine::LinkCMet(const std::string& cmet,void(*f)(float*)){
    m_cMetLinks[m_program.linkTable.at(cmet)] = f;
}

void DmslUnit::SetAttribute(const std::string& attribute,float f){
    m_attribute[m_dvm->m_program.linkTable.at(attribute)] = f;
}

float DmslUnit::GetAttribute(const std::string& attribute)
{
    return m_attribute[m_dvm->m_program.linkTable.at(attribute)];
}

Dmsl::DmslUnit::DmslUnit(const DmslVirtualMachine& dvm,uint32_t unit){
    if(!dvm.Good()) throw runtime_error("Dmsl虚拟机中的代码不正确！");
    if(!dvm.HaveUnit(unit)) throw runtime_error("没有在Dmsl虚拟机中找到对应程序单元！");
    m_attribute.resize(dvm.m_program.units.at(unit).memSize);
    m_dvm = &dvm;
    m_unit = unit;
    dvm.runCode(dvm.m_program.units.at(unit).initProgram,&m_attribute);
}

static float GetFloat(const std::vector<uint8_t>& code,uint32_t& address){
    float a;
    char* p = (char*)&a;
    for(uint32_t i = 0;i < sizeof(float);++i)
        p[i] = code[address++];
    #ifdef _OUT_ASM
    cout<<"  "<<a;
    #endif
    return a;
};

static uint32_t GetAddress(const std::vector<uint8_t>& code,uint32_t& address){
    uint32_t a;
    char* p = (char*)&a;
    for(uint32_t i = 0;i < sizeof(uint32_t);++i)
        p[i] = code[address++];
    #ifdef _OUT_ASM
    cout<<"  (ADDRESS)"<<a;
    #endif
    return a;
};

#include <stack>

#define PUSH_FLOAT(x) d.f = (x);stk.push(d);
#define PUSH_BOOL(x) d.b = (x);stk.push(d);
#define PUSH_ADDRESS(x) d.a = (x);stk.push(d);

#define POP_FLOAT() stk.top().f;stk.pop();
#define POP_BOOL() stk.top().b;stk.pop();
#define POP_ADDRESS() stk.top().a;stk.pop();

void DmslVirtualMachine::runCode(const std::vector<uint8_t>& code,std::vector<float>* attrMem) const{
    uint32_t address = 0;
    union data{float f;uint32_t a;bool b;}d;
    std::stack<data> stk;

    while(1){
        using Dmsl::VirtualMachine::ElfCode;
        #ifdef _OUT_ASM
        cout<<endl<<(int)code[address];
        #endif
        switch((Dmsl::VirtualMachine::ElfCode)code[address++]){
        case ElfCode::DMOVFS:{
                uint32_t varAddress = GetAddress(code,address);
                auto srcCount = GetAddress(code,address);
                for(int i = 0;i < m_dmovfs_sel;++i) GetFloat(code,address);
                attrMem ->at(varAddress) = GetFloat(code,address);
                for(uint32_t i = 0;i < srcCount - m_dmovfs_sel - 1;++i) GetFloat(code,address);
            }
            break;
        case ElfCode::MOV:
            attrMem -> at(GetAddress(code,address)) = POP_FLOAT();
            break;
        case ElfCode::CFUNC:{
                auto funcInLinkTable = GetAddress(code,address);
                auto func = m_cFuncLinks[funcInLinkTable];
                std::vector<float> params;
                auto paramSize = m_program.funcParamNum.at(funcInLinkTable);
                for(uint32_t i = 0;i < paramSize;++i){
                    params.push_back(stk.top().f);
                    stk.pop();
                }
				if (!params.empty()) {
					PUSH_FLOAT(func(&params[0]));
				}
				else { PUSH_FLOAT(func(nullptr)); }
            }
            break;
        case ElfCode::CMET:{
                auto metInLinkTable = GetAddress(code,address);
                auto func = m_cMetLinks[metInLinkTable];
                std::vector<float> params;
                auto paramSize = m_program.metParamNum.at(metInLinkTable);
                for(uint32_t i = 0;i < paramSize;++i){
                    params.push_back(stk.top().f);
                    stk.pop();
                }
                if(!params.empty())func(&params[0]);
				else func(nullptr);
            }
            break;
        case ElfCode::PUSH:
            PUSH_FLOAT(GetFloat(code,address));
            break;
        case ElfCode::IF:
            if(!stk.top().b) address = GetAddress(code,address);
            else GetAddress(code,address);
            stk.pop();
            break;
        case ElfCode::JMP:
            address = GetAddress(code,address);
            break;
        case ElfCode::END:
            return;
            break;
        case ElfCode::DUNPA:
            PUSH_FLOAT(attrMem ->at(GetAddress(code,address)));
            break;
        case ElfCode::DUNPU:
            PUSH_FLOAT(m_uniform[GetAddress(code,address)]);
            break;
        case ElfCode::NEGA:{
                auto m = POP_FLOAT();
                PUSH_FLOAT(-m);
            }
            break;
        case ElfCode::ADD:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_FLOAT(m1+m2);
            }
            break;
        case ElfCode::SUB:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_FLOAT(m2-m1);
            }
            break;
        case ElfCode::MUL:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_FLOAT(m2*m1);
            }
            break;
        case ElfCode::DIV:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_FLOAT(m2/m1);
            }
            break;
        case ElfCode::MOD:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_FLOAT(float((int)m2%(int)m1));
            }
            break;
        case ElfCode::BIG:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_BOOL(m2>m1);
            }
            break;
        case ElfCode::BIGE:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_BOOL(m2>=m1);
            }
            break;
        case ElfCode::SML:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_BOOL(m2<m1);
            }
            break;
        case ElfCode::SMLE:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_BOOL(m2<=m1);
            }
            break;
        case ElfCode::EQ:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_BOOL(m2==m1);
            }
            break;
        case ElfCode::NE:{
                auto m1 = POP_FLOAT();
                auto m2 = POP_FLOAT();
                PUSH_BOOL(m2!=m1);
            }
            break;
        case ElfCode::AND:{
                auto m1 = POP_BOOL();
                auto m2 = POP_BOOL();
                PUSH_BOOL(m2&&m1);
            }
            break;
        case ElfCode::OR:{
                auto m1 = POP_BOOL();
                auto m2 = POP_BOOL();
                PUSH_BOOL(m2||m1);
            }
            break;
        case ElfCode::NOT:{
                auto m1 = POP_BOOL();
                PUSH_BOOL(!m1);
            }
            break;
        };
    }
}

