#include "Options.h"
#include <list>

int main(int argc, const char *argv[]) {
	Option<int> number('n', "number", "some number", 0);
	Option<int> number2('k', "kumber", "some other number", 0, {0, 3, 3});
	OptionMap<int> numbers('m', "numbers", "several numbers");
	OptionMap<std::string> strings('s', "strings", "several strings");
	Option<const char *>cs('c', "cstring", "a c string", nullptr, {"bla", "blubb"});
	Option<std::string>Cs('C', "Cstring", "a C++ string", "",  {"bla", "blubb"});
	OptionContainer<double> dnums('d', "doubles", "double numbers");
	OptionContainer<const char*, std::list<const char*>> stringl('S', "listString", "list of strings");
	OptionContainer<std::string, std::list<std::string>> stringS('x', "liststring", "list of std::strings");
	OptionParser parser("option parsing example");
	auto unusedOptions = parser.fParse(argc, argv);

	for (auto & unusedOption : unusedOptions) {
		std::cout << "unused option : '" << unusedOption << "'" << std::endl;
	}
	for (auto & num : numbers) {
		std::cout << " number '" << num.first << "' is '" << num.second << "'\n";
	}
	for (auto & str : strings) {
		std::cout << " string '" << str.first << "' is '" << str.second << "'\n";
	}
	for (double & dnum : dnums) {
		std::cout << " double number '" << dnum << "\n";
	}
	for (auto & it : stringl) {
		std::cout << " list string '" << it << "\n";
	}
	for (auto & it : stringS) {
		std::cout << " list std::string '" << it << "\n";
	}
	return number;
}
