#include "DMSL.h"
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;
using namespace Dmsl;

float add(float* argv){
    cout<<endl<<"ADD:"<<argv[0]<<","<<argv[1]<<endl;
    return argv[0]+argv[1];
}

int main() {

	auto file = fopen("0.txt", "rb");
	fseek(file, 0, SEEK_END);
	auto size = ftell(file);
	fseek(file, 0, SEEK_SET);
	string s;
	char buf[65536] = { '\0' };
	fread(buf, size, 1, file);
	s += buf;

	cout<<"Compiler:"<<endl;
    DmslVirtualMachine vm(s,cout);
    cout<<endl<<"Result:"<<endl;
    vm.SetDMOVFSSelect(3);
    vm.SetUniform("u",1.234);
    vm.LinkCFunc("add",&add);

    DmslUnit dun(vm,1);
    dun.Call();
    cout<<endl<<"OUT:"<<dun.GetAttribute("a1");

	//system("pause");

	return 0;
}
