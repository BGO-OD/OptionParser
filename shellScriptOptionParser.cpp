#include "Options.h"
#include <set>
#include <limits>
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
		          ". <(shellScriptOptionParser $(cd $(dirname $0); echo \"$(pwd)/$(basename $0)\") \"$@\"<<EOF\n"
		          "description of script\n"
		          "options:\n"
		          "<options>\n"
		          "trailer:\n"
		          "trailing rest of explanation\n"
		          "EOF\n"
		          "test $? != 0 && echo exit\n"
		          ")\n"
		          "Option sytax is:\n"
		          "[export] type shortOpt longOpt default descripton\n"
		          "\ttype may be one of int, uint bool or string\n"
		          "\tshortOpt is the one-letter variant, use - to have none\n"
		          "\tlongOpt is the long variant and the name of the shell variable\n"
		          "\tdefault is the default value (surprise, surprise!)\n"
		          "\tthe rest of the line is the description\n"
		          "\tif the next line starts with range the values following are added to the\n"
		          "\tallowed value range of the option, many range lines may follow!\n"
		          "\tif only two are given they denote a true range in the closed interval\n"
		          "\tThe keyword minusMinusSpecialTreatment will put the parameters\n"
		          "\tfollowing -- into the shell variable following that keyword\n"
		          "\tThe keyword noPath clears the search path for config files\n"
		          "\tThe keyword path adds the (escaped) rest of the line\n"
		          "\tThe keyword minUnusedParameters sets the min number of params\n"
		          "\tThe keyword maxUnusedParameters sets the max number of params\n"
		          "\tto the search path for config files\n";
		return (1);
	}

	std::vector<OptionBase*> options;
	std::set<const OptionBase*> exportedOptions;
	std::string minusMinusSpecialTreatment = "";
	std::vector<std::string> searchPath({"/etc/", "~/.", "~/.config/", "./."});

	unsigned int minUnusedParameters = 0;
	unsigned int maxUnusedParameters = std::numeric_limits<unsigned int>::max();
	{
		std::string keyWord;
		bool exportNextOption = false;
		while (std::cin.good()) {
			std::cin >> keyWord;
			if (std::cin.eof()) {
				break;
			}
			if (keyWord == "string") {
				options.push_back(fOptionFromStream<std::string>(std::cin));
			} else if (keyWord == "int") {
				options.push_back(fOptionFromStream<int>(std::cin));
			} else if (keyWord == "uint") {
				options.push_back(fOptionFromStream<int>(std::cin));
			} else if (keyWord == "bool") {
				options.push_back(fOptionFromStream<bool>(std::cin));
			} else if (keyWord == "range") {
				options.back()->fAddToRangeFromStream(std::cin);
			} else if (keyWord == "export") {
				exportNextOption = true;
				continue;
			} else if (keyWord == "minusMinusSpecialTreatment") {
				std::cin >> minusMinusSpecialTreatment;
				continue;
			} else if (keyWord == "minUnusedParameters") {
				std::cin >> minUnusedParameters;
				continue;
			} else if (keyWord == "maxUnusedParameters") {
				std::cin >> maxUnusedParameters;
				continue;
			} else if (keyWord == "noPath") {
				searchPath.clear();
				continue;
			} else if (keyWord == "path") {
				std::string buffer;
				std::getline(std::cin, buffer);
				char buffer2[buffer.length() + 1];
				OptionParser::fReCaptureEscapedString(buffer2, buffer.c_str());
				searchPath.push_back(buffer2);
			} else if (keyWord == "trailer:") {
				break;
			} else {
				std::cerr << "illegal option type '" << keyWord << "', giving up" << std::endl;
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

	OptionParser parser(description.c_str(), trailer.c_str(), searchPath);
	parser.fSetMessageStream(&std::cerr);
	parser.fSetHelpReturnValue(1);
	parser.fSetExecutableName(argv[1]);
	if (! minusMinusSpecialTreatment.empty()) {
		parser.fSetMinusMinusStartsExtraList();
	}

	auto unusedOptions = parser.fParse(argc - 1, argv + 1);

	if (unusedOptions.size() < minUnusedParameters ||
	        unusedOptions.size() > maxUnusedParameters) {
		std::cerr << "illegal number of non-option parameters " << unusedOptions.size() << ", must be between " << minUnusedParameters << " and " << maxUnusedParameters << std::endl;
		parser.fHelp();
		return (1);
	}


	for (auto option : options) {
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
		for (auto & unusedOption : unusedOptions) {
			std::cout << " " << unusedOption;
		}
		std::cout << "\n";
	}
	if (parser.fGetStuffAfterMinusMinus().empty() == false) {
		std::cout << minusMinusSpecialTreatment << "=\"";
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
