#pragma once
#include <ostream>
#include <string>
#include <vector>
#include <cstdint>
#include <map>

namespace Dmsl{
    namespace Compiler{
		struct Compiled {
			struct Unit {
				uint32_t memSize;
				std::vector<uint8_t> initProgram, mainProgram;
			};
			std::map<std::string, uint32_t> linkTable;
			std::map<uint32_t,uint32_t> metParamNum,funcParamNum;
			std::map<uint32_t, Unit> units;
			uint32_t uniformSize,cFuncSize,cMetSize;
		};
    };
    class DmslUnit;

    class DmslVirtualMachine{ //Dmsl虚拟机
    friend class DmslUnit;
    private:
        Compiler::Compiled m_program;
        std::vector<void(*)(float*)> m_cMetLinks;
        std::vector<float(*)(float*)> m_cFuncLinks;
        std::vector<float> m_uniform;

        bool m_good = false;
        int m_dmovfs_sel = 0;

        void runCode(const std::vector<uint8_t>& code,std::vector<float>* attrMem) const;
    public:
        DmslVirtualMachine(const std::string& programCode,std::ostream& log);    //传入程序代码和日志输出流以编译代码到虚拟机
        inline bool Good() const {return m_good;}; //是否状态良好
        inline bool HaveUnit(uint32_t u) const {return m_program.units.count(u)==1;}
        void SetUniform(const std::string& uniform,float);  //设置Uniform变量
        void LinkCFunc(const std::string& cfunc,float(*)(float*));  //链接C函数
        void LinkCMet(const std::string& cmet,void(*)(float*));    //链接C方法
        inline void SetDMOVFSSelect(int select){m_dmovfs_sel = select;} //多重赋值时使用哪个值，从0开始
    };

    class DmslUnit{ //执行单元
    friend class DmslVirtualMachine;
    private:
        std::vector<float> m_attribute;
        const DmslVirtualMachine* m_dvm;
        uint32_t m_unit;
    public:
        DmslUnit(const DmslVirtualMachine& dvm,uint32_t unit);
        inline void Call(){m_dvm -> runCode(m_dvm->m_program.units.at(m_unit).mainProgram,&m_attribute);} //运行程序
        void SetAttribute(const std::string& attribute,float);  //设置Attribute变量
        float GetAttribute(const std::string& attribute);   //获取Attribute变量
    };
}

