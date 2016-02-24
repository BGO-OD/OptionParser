#include "Options.h"
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
		          "\tcommands that set variables to the parsed options to standard output\n";
		return (1);
	}

	std::vector<OptionBase*> options;
	{
		std::string optionType;
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
			} else if (optionType == "trailer:") {
				break;
			} else {
				std::cerr << "illegal option type '" << optionType << "', giving up" << std::endl;
				return 1;
			}
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
		std::cout << (*it)->fGetLongName() << "=";
		(*it)->fWriteValue(std::cout);
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

	return 0;
}
