#include "Options.h"

int main(int argc, const char *argv[]) {
	Option<int> number('n', "number", "some number", 0);

	OptionParser parser("option parsing example");
	auto unusedOptions = parser.fParse(argc, argv);

	for (auto it = unusedOptions.begin(); it != unusedOptions.end(); ++it) {
		std::cout << "unused option :" << *it << std::endl;
	}
	return number;
}
