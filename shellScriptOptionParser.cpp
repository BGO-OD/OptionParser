#include "Options.h"


int main(int argc, const char *argv[]) {
	std::string description;
	while (std::cin.good()) {
		std::string line;
		std::getline(std::cin, line);
		if (line.compare("options:")==0) {
			break;
		}
		description += line;
		description += "\n";
	}
	

	OptionParser parser(description.c_str());
	parser.fSetMessageStream(&std::cerr);
	parser.fSetHelpReturnValue(1);
	

	std::vector<Option<std::string>*> options;
	{
		char shortName;
		std::string longName;
		std::string defaultValue;
		std::string description;
		
		while (std::cin.good()) {
			std::cin >> shortName;
			if (std::cin.eof()) {
				break;
			}
			std::cin >> longName;
			std::cin >> defaultValue;
			std::getline(std::cin, description);

			if (shortName == '-') {
				shortName = '\0';
			}

			options.push_back(new Option<std::string>(shortName,longName.c_str(),description.c_str(),defaultValue));
		}
	}
	auto unusedOptions = parser.fParse(argc-1,argv+1);
	
	for (auto it=options.begin(); it!=options.end(); it++) {
		std::cout << (*it)->fGetLongName() << "=" << (*it)->fGetValue() << "\n";
	}
	for (unsigned int i=0; i<unusedOptions.size(); i++) {
		std::cout << "cmdLine[" << i << "]=" << unusedOptions[i] << "\n";
	}

	return 0;
}
