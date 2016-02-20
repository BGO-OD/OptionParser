#include "Options.h"

int main(int argc, const char *argv[]) {
	Option<int> number('n', "number", "some number", 0);
	Option<int> number2('k', "kumber", "some other number", 0, new std::vector<int> {0, 3, 3});
	OptionMap<int> numbers('m', "numbers", "several numbers");
	OptionMap<std::string> strings('s', "strings", "several strings");
	Option<const char *>cs('c', "cstring", "a c string", NULL, new std::vector<const char *> {"bla", "blubb"});
	Option<std::string>Cs('C', "Cstring", "a C++ string", "", new std::vector<std::string> {"bla", "blubb"});

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
	return number;
}
