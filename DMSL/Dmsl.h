#pragma once
#include <ostream>
#include <string>
#include <vector>
#include "DmslCompiler.h"

namespace Dmsl{
    class DmslUnit;
    
    class DmslVirtualMachine final{ //Dmsl虚拟机
    friend class DmslUnit;
    private:
        Compiler::Compiled m_program;
        (void(*)(float*))* m_cMetLinks;
        (float(*)(float*))* m_cFuncLinks;
        float* m_uniform;
    public:
        DmslVirtualMachine(const std::string& programCode,ostream& log);    //传入程序代码和日志输出流以编译代码到虚拟机
        void SetUniform(const std::string& uniform,float);  //设置Uniform变量
        void LinkCFunc(const std::string& cfunc,float(*)(float*));  //链接C函数
        void LinkCMet(const std::string& cfunc,void(*)(float*));    //链接C方法
    };
    
    class DmslUnit final{   //Dmsl运行单元
    friend class DmslVirtualMachine;
    private:
        const DmslVirtualMachine* m_vm;
        int m_unit;
        float* m_attrs;
        
    public:
        DmslUnit(const DmslVirtualMachine& vm,int unit);    //传入虚拟机和程序单元编号以创建运行单元
        ~DmslUnit();
        void Run(); //运行程序
        void SetAttribute(const std::string& attribute,float);  //设置Attribute变量
        float GetAttribute(const std::string& attribute);   //获取Attribute变量
    };
}