#include "Options.h"
#include <list>

int main(int argc, const char *argv[]) {
	Option<int> number('n', "number", "some number", 0);
	Option<int> number2('k', "kumber", "some other number", 0, {0, 3, 3});
	OptionMap<int> numbers('m', "numbers", "several numbers");
	OptionMap<std::string> strings('s', "strings", "several strings");
	Option<const char *>cs('c', "cstring", "a c string", NULL, {"bla", "blubb"});
	Option<std::string>Cs('C', "Cstring", "a C++ string", "",  {"bla", "blubb"});
	OptionContainer<double> dnums('d', "doubles", "double numbers");
	OptionContainer<const char*, std::list<const char*>> stringl('S', "listString", "list of strings");
	OptionContainer<std::string, std::list<std::string>> stringS('x', "liststring", "list of std::strings");
	OptionParser parser("option parsing example");
	auto unusedOptions = parser.fParse(argc, argv);

	for (auto it = unusedOptions.begin(); it != unusedOptions.end(); ++it) {
		std::cout << "unused option : '" << *it << "'" << std::endl;
	}
	for (auto it = numbers.begin(); it != numbers.end(); ++it) {
		std::cout << " number '" << it->first << "' is '" << it->second << "'\n";
	}
	for (auto it = strings.begin(); it != strings.end(); ++it) {
		std::cout << " string '" << it->first << "' is '" << it->second << "'\n";
	}
	for (auto it = dnums.begin(); it != dnums.end(); ++it) {
		std::cout << " double number '" << *it << "\n";
	}
	for (auto it = stringl.begin(); it != stringl.end(); ++it) {
		std::cout << " list string '" << *it << "\n";
	}
	for (auto it = stringS.begin(); it != stringS.end(); ++it) {
		std::cout << " list std::string '" << *it << "\n";
	}
	return number;
}
