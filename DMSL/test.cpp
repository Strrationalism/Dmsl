#include "DMSLUtils.h"
#include "DMSLPrecompiler.h"
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;
using namespace Dmsl::Utils;
int main() {
	auto file = fopen("0.txt", "rb");
	fseek(file, 0, SEEK_END);
	auto size = ftell(file);
	fseek(file, 0, SEEK_SET);
	string s;
	char buf[65536] = { '\0' };
	fread(buf, size, 1, file);
	s += buf;

	Dmsl::Compiler::Precompiled p;
	Dmsl::Compiler::Precompile(s, p,cout);
	Dmsl::Compiler::DisplayPrecompiled(p, cout);

	system("pause");

	return 0;
}