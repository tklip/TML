#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include <map>
#include "defs.h"
#include "output.h"

using namespace std;
wostream wcnull(0);
map<std::wstring, output*> output::outputs = {};
wstring output::named = L"";

wostream& operator<<(wostream& os, const output& o) {
	return os << o.target();
}

wstring s2ws(const std::string& s) {
	return std::wstring_convert<std::codecvt_utf8<wchar_t>>()
		.from_bytes(s);
}
string ws2s(const wstring& s) {
	return std::wstring_convert<std::codecvt_utf8<wchar_t>>()
		.to_bytes(s);
}

int main() {

	// create various outputs
	output::create(L"null",     L".null",   L"@null");
	output::create(L"output",   L".out",    L"@stdout");
	output::create(L"errors",   L".err",    L"@stderr");
	output::create(L"buffer",   L".buff",   L"@buffer");
	output::create(L"tmp",      L".tmp",    L"@tmp");
	output::create(L"file",     L".file",   L"test.file");
	output::create(L"noname", L".noname", L"@name");
	output::set_name(L"named");
	output::create(L"name1",    L".name1",  L"@name");
	output::create(L"name2",    L".name2",  L"@name");

	// output to null
	output::to(L"null") << L"null test" << endl;

	// output to stdout
	output::to(L"output") << L"stdout test" << endl;

	// output to stderr
	output::to(L"errors") << L"stderr test" << endl;

	// output to buffer
	output::to(L"buffer") << L"buffer test";
	// read buffer content
	wcout<<L"buffer content: \""<<output::read(L"buffer")<<L'"'<<endl;
	assert(output::read(L"buffer") == L"buffer test");

	// output to tmp file
	output::to(L"tmp") << L"tmp test" << endl;

	// output to file
	output::to(L"file") << L"file test" << endl;

	// output to named file w/o specifying the name => tmp file
	output::to(L"noname") << L"noname test" << endl;

	// output to named files
	output::to(L"name1") << L"named1 test" << endl;
	output::to(L"name2") << L"named2 test" << endl;

	// get known
	wcout<<L"get returns output*: "<<output::get(L"output")<<endl;
	assert(output::get(L"output"));

	// get unkown
	wcout<<L"get unknown output: "<<output::get(L"not created")<<endl;
	assert(!output::get(L"not created"));

	// get filename outputs target to
	wcout << L"outputs to files:" << endl;
	wcout << L"\t null: "   << output::file(L"null")   << endl;
	wcout << L"\t output: " << output::file(L"output") << endl;
	wcout << L"\t errors: " << output::file(L"errors") << endl;
	wcout << L"\t buffer: " << output::file(L"buffer") << endl;
	wcout << L"\t tmp: "    << output::file(L"tmp")    << endl;
	wcout << L"\t noname: " << output::file(L"noname") << endl;
	wcout << L"\t name1: "  << output::file(L"name1")  << endl;
	wcout << L"\t name2: "  << output::file(L"name2")  << endl;
	wcout << L"\t file: "   << output::file(L"file")   << endl;
	assert(output::file(L"null")   == L"");
	assert(output::file(L"output") == L"");
	assert(output::file(L"errors") == L"");
	assert(output::file(L"buffer") == L"");
	assert(output::file(L"tmp")    != L"");
	assert(output::file(L"noname") != L"");
	assert(output::file(L"name1")  == L"named.name1");
	assert(output::file(L"name2")  == L"named.name2");
	assert(output::file(L"file")   == L"test.file");

	// is null
	wcout << L"is_null?:" << endl;
	wcout << L"\t null: "   << output::is_null(L"null")   << endl;
	wcout << L"\t output: " << output::is_null(L"output") << endl;
	wcout << L"\t errors: " << output::is_null(L"errors") << endl;
	wcout << L"\t buffer: " << output::is_null(L"buffer") << endl;
	wcout << L"\t tmp: "    << output::is_null(L"tmp")    << endl;
	wcout << L"\t noname: " << output::is_null(L"noname") << endl;
	wcout << L"\t name1: "  << output::is_null(L"name1")  << endl;
	wcout << L"\t name2: "  << output::is_null(L"name2")  << endl;
	wcout << L"\t file: "   << output::is_null(L"file")   << endl;
	assert( output::is_null(L"null"));
	assert(!output::is_null(L"output"));
	assert(!output::is_null(L"errors"));
	assert(!output::is_null(L"buffer"));
	assert(!output::is_null(L"tmp"));
	assert(!output::is_null(L"noname"));
	assert(!output::is_null(L"name1"));
	assert(!output::is_null(L"name2"));
	assert(!output::is_null(L"file"));

	// get targets
	wcout << L"targets:" << endl;
	wcout << L"\t null: "   << output::get_target(L"null")   << endl;
	wcout << L"\t output: " << output::get_target(L"output") << endl;
	wcout << L"\t errors: " << output::get_target(L"errors") << endl;
	wcout << L"\t buffer: " << output::get_target(L"buffer") << endl;
	wcout << L"\t tmp: "    << output::get_target(L"tmp")    << endl;
	wcout << L"\t noname: " << output::get_target(L"noname") << endl;
	wcout << L"\t name1: "  << output::get_target(L"name1")  << endl;
	wcout << L"\t name2: "  << output::get_target(L"name2")  << endl;
	wcout << L"\t file: "   << output::get_target(L"file")   << endl;
	assert(output::get_target(L"null")   == L"@null");
	assert(output::get_target(L"output") == L"@stdout");
	assert(output::get_target(L"errors") == L"@stderr");
	assert(output::get_target(L"buffer") == L"@buffer");
	assert(output::get_target(L"tmp")    == L"@tmp");
	assert(output::get_target(L"noname") == L"@name");
	assert(output::get_target(L"name1")  == L"@name");
	assert(output::get_target(L"name2")  == L"@name");
	assert(output::get_target(L"file")   == L"test.file");
}
