#include "Options.h"

int main(int argc, const char *argv[]) {
	Option<int> number('n', "number", "some number", 0);
	OptionMap<int> numbers('m', "numbers", "several numbers");
	OptionMap<std::string> strings('s', "strings", "several strings");


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
