#include "Options.h"
#include <set>
#include <unistd.h>

template <typename T> Option<T>* fOptionFromStream(std::istream &aStream) {
	char shortName;
	std::string longName;
	T defaultValue;
	std::string description;

	aStream >> shortName;
	aStream >> longName;
	aStream >> std::boolalpha >> defaultValue;
	std::getline(aStream, description);

	if (shortName == '-') {
		shortName = '\0';
	}
	return new Option<T>(shortName, longName.c_str(), description.c_str(), defaultValue);
}

int main(int argc, const char *argv[]) {
	std::string description;
	if (isatty(0) != 1) {
		while (std::cin.good()) {
			std::string line;
			if (std::cin.eof()) {
				break;
			}
			std::getline(std::cin, line);
			if (line.compare("options:") == 0) {
				break;
			}
			description += line;
			description += "\n";
		}
	}
	if (description.empty() || description == "\n") {
		std::cout << argv[0] << ": shell script option parser.\n"
		          "\treads description of options from stdandard input and writes shell\n"
		          "\tcommands that set variables to the parsed options to standard output\n"
		          "typical invocation looks like this:\n"
		          ". <(shellScriptOptionParser $0 \"$@\"<<EOF\n"
		          "description of script\n"
		          "options:\n"
		          "export int n number 55 an example exported integer option\n"
		          "trailer:\n"
		          "trailing rest of explanation\n"
		          "EOF\n"
		          "test $? != 0 && echo exit\n"
		          ")\n";
		return (1);
	}

	std::vector<OptionBase*> options;
	std::set<const OptionBase*> exportedOptions;
	{
		std::string optionType;
		bool exportNextOption = false;
		while (std::cin.good()) {
			std::cin >> optionType;
			if (std::cin.eof()) {
				break;
			}
			if (optionType == "string") {
				options.push_back(fOptionFromStream<std::string>(std::cin));
			} else if (optionType == "int") {
				options.push_back(fOptionFromStream<int>(std::cin));
			} else if (optionType == "uint") {
				options.push_back(fOptionFromStream<int>(std::cin));
			} else if (optionType == "bool") {
				options.push_back(fOptionFromStream<bool>(std::cin));
			} else if (optionType == "range") {
				options.back()->fAddToRangeFromStream(std::cin);
			} else if (optionType == "export") {
				exportNextOption = true;
				continue;
			} else if (optionType == "trailer:") {
				break;
			} else {
				std::cerr << "illegal option type '" << optionType << "', giving up" << std::endl;
				return 1;
			}
			if (exportNextOption) {
				exportedOptions.insert(options.back());
			}
			exportNextOption = false;
		}
	}
	std::string trailer;
	while (std::cin.good()) {
		std::string line;
		if (std::cin.eof()) {
			break;
		}
		std::getline(std::cin, line);
		trailer += line;
		trailer += "\n";
	}

	OptionParser parser(description.c_str(), trailer.c_str());
	parser.fSetMessageStream(&std::cerr);
	parser.fSetHelpReturnValue(1);


	auto unusedOptions = parser.fParse(argc - 1, argv + 1);

	for (auto it = options.begin(); it != options.end(); it++) {
		auto option = *it;
		if (exportedOptions.find(option) != exportedOptions.end()) {
			std::cout << "export ";
		}
		std::cout << option->fGetLongName() << "=";
		option->fWriteValue(std::cout);
		std::cout << "\n";
	}
	std::cout << "shift $#\n";
	if (unusedOptions.empty() == false) {
		std::cout << "set --";
		for (unsigned int i = 0; i < unusedOptions.size(); i++) {
			std::cout << " " << unusedOptions[i];
		}
		std::cout << "\n";
	}
	if (parser.fGetStuffAfterMinusMinus().empty() == false) {
		std::cout << "AfterMinusMinus=\"";
		for (auto it =  parser.fGetStuffAfterMinusMinus().begin(); it != parser.fGetStuffAfterMinusMinus().end(); ++it) {
			if (it != parser.fGetStuffAfterMinusMinus().begin()) {
				std::cout << " ";
			}
			std::cout << *it;
		}
		std::cout << "\"\n";
	}

	return 0;
}
