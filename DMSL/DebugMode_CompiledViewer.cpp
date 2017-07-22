#include "DMSL.h"
#include <math.h>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;
using namespace Dmsl;

static DmslVirtualMachine* pVm;

double dmslSin(double* p, void*){
    return sin(*p);
}

double dmslCos(double* p, void*){
    return cos(*p);
}

double dmslTan(double* p, void*){
    return tan(*p);
}

double dmslSqrt(double* p, void*){
    return sqrt(*p);
}

double dmslRand(double*, void*){
    return rand();
}

double dmslAbs(double* p, void*){
    return fabs(*p);
}

double dmslPow(double* p, void*){
    return pow(p[1],p[0]);
}

void dmslPrint(double* p, void*){
    cout<<endl<<"DMSLPrint:"<<*p;
}

void dmslEndl(double* p, void*){
    cout<<endl;
}

void dmslSetDMOVFS(double *p, void*){
    pVm -> SetDMOVFSSelect(static_cast<int>(*p));
}

void dmslGensokyo(double* p,void*){
    cout<<endl<<"×ÏÀÏÌ«ÆÅjadpjnznlcNSCMmfncvnsv"<<endl;
}

int main() {
	FILE* file;
	fopen_s(&file,"program.dmsl", "rb");
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

	vm.Ready();

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

