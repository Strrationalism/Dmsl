#include "DMSL.h"
#include <math.h>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;
using namespace Dmsl;

static DmslVirtualMachine* pVm;

float dmslSin(float* p){
    return sin(*p);
}

float dmslCos(float* p){
    return cos(*p);
}

float dmslTan(float* p){
    return tan(*p);
}

float dmslSqrt(float* p){
    return sqrt(*p);
}

float dmslRand(float*){
    return rand();
}

float dmslAbs(float* p){
    return fabs(*p);
}

float dmslPow(float* p){
    return pow(p[1],p[0]);
}

void dmslPrint(float* p){
    cout<<endl<<"DMSLPrint:"<<*p;
}

void dmslEndl(float* p){
    cout<<endl;
}

void dmslSetDMOVFS(float *p){
    pVm -> SetDMOVFSSelect(*p);
}

void dmslGensokyo(float* p){
    cout<<endl<<"×ÏÀÏÌ«ÆÅjadpjnznlcNSCMmfncvnsv"<<endl;
}

int main() {

	auto file = fopen("program.dmsl", "rb");
	fseek(file, 0, SEEK_END);
	auto size = ftell(file);
	fseek(file, 0, SEEK_SET);
	string s;
	char buf[65536] = { '\0' };
	fread(buf, size, 1, file);
	s += buf;

	cout<<"Compiler:"<<endl;
    DmslVirtualMachine vm(s,cout);
    cout<<"Compiled!"<<endl;
    pVm = &vm;

    cout<<"Linking...";
    vm.SetUniform("dmslVersion",160522);

    vm.LinkCFunc("sin",&dmslSin);
    vm.LinkCFunc("cos",&dmslCos);
    vm.LinkCFunc("tan",&dmslTan);
    vm.LinkCFunc("sqrt",&dmslSqrt);
    vm.LinkCFunc("abs",&dmslAbs);
    vm.LinkCFunc("pow",&dmslPow);
    vm.LinkCFunc("rand",&dmslRand);

    vm.LinkCMet("print",&dmslPrint);
    vm.LinkCMet("endl",&dmslEndl);
    vm.LinkCMet("setDMOVFS",&dmslSetDMOVFS);
    vm.LinkCMet("gensokyo",&dmslGensokyo);

    cout<<"OK"<<endl;

    map<uint32_t,DmslUnit*> us;
    while(1){
        cout<<endl<<"Which unit do you want to call?"<<endl;
        cout<<"Unit number:";
        uint32_t unit;
        cin>>unit;
        cout<<endl;
        if(us.count(unit) == 0){
            if(vm.HaveUnit(unit)){
                cout<<endl<<"---------DMSL Log----------"<<endl;
                auto dunit = new DmslUnit(vm,unit);
                dunit->Call();
                us[unit] = dunit;
                cout<<"---------DMSL Log End----------"<<endl;
                cout<<"Result:"<<dunit->GetAttribute("result")<<endl<<endl;
            }else{
                cout<<"Have not this unit."<<endl;
            }
        }else{
            cout<<endl<<"---------DMSL Log----------"<<endl;
            us[unit]->Call();
            cout<<"---------DMSL Log End----------"<<endl;
            cout<<"Result:"<<us[unit]->GetAttribute("result")<<endl<<endl;
        }
    }

	return 0;
}

