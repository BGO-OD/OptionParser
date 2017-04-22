/*
   Option parser  C++(11) library for parsing command line options
    Copyright (C) 2016  Juergen Hannappel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "OptionsChrono.h"
#include <limits>
#include <ratio>
#include <set>
#include <unistd.h>

template <typename T> class arrayOption: public  options::container<T> {
  public:
	template <class ... Types> arrayOption(Types ... args) :
		options::container<T>(args...) {
	};
	void fWriteValue(std::ostream& aStream) const override {
		aStream << "(";
		for (const auto& item : *this) {
			aStream << ' ';
			using options::escapedIO::operator<<;
			aStream << item;
		}
		aStream << ")";
	};
};

template <typename T> class mapOption: public  options::map<T> {
  public:
	template <class ... Types> mapOption(Types ... args) :
		options::map<T>(args...) {
	};
	void fWriteValue(std::ostream& aStream) const override {
		aStream << "; unset " << this->fGetLongName() << "; declare -A " << this->fGetLongName() << "=(";
		for (const auto& item : *this) {
			aStream << " [" << item.first << "]=";
			using options::escapedIO::operator<<;
			aStream << item.second;
		}
		aStream << " )";
	};
};

template <typename T> class listOption: public  options::container<T> {
  public:
	template <class ... Types> listOption(Types ... args) :
		options::container<T>(args...) {
	};
	void fWriteValue(std::ostream& aStream) const override {
		aStream << "\"";
		bool start = true;
		for (const auto& item : *this) {
			if (!start) {
				aStream << ' ';
			}
			start = false;
			using options::escapedIO::operator<<;
			aStream << item;
		}
		aStream << "\"";
	};
};

enum typeModifierType {
	kSimple,
	kAsArray,
	kAsMap,
	kAsList
};

template <typename T> options::base* fOptionFromStream(std::istream &aStream, T defaultValue, typeModifierType aAsWhat) {
	char shortName;
	std::string longName;
	std::string description;

	aStream >> shortName;
	aStream >> longName;
	aStream.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
	std::getline(aStream, description);

	if (shortName == '-') {
		shortName = '\0';
	}
	switch (aAsWhat) {
		case kSimple:
			return new options::single<T>(shortName, longName, description, defaultValue);
		case kAsArray:
			return new arrayOption<T>(shortName, longName, description);
		case kAsMap:
			return new mapOption<T>(shortName, longName, description);
		case kAsList:
			return new listOption<T>(shortName, longName, description);
	}
	return nullptr;
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
		          ". <(shellScriptOptionParser $\"(cd \"$(dirname \"$0\")\"; echo \"$(pwd)/$(basename \"$0\")\")\" \"$@\"<<EOF\n"
		          "description of script\n"
		          "options:\n"
		          "<options>\n"
		          "trailer:\n"
		          "trailing rest of explanation\n"
		          "EOF\n"
		          "test $? != 0 && echo exit\n"
		          ")\n"
		          "Option sytax is:\n"
		          "[export|array|map|list][positional number] type shortOpt longOpt descripton\n"
		          "\ttype may be one of 'int', 'uint', 'bool' or 'string'\n"
		          "\tfor durations the type 'seconds' is provided\n"
		          "\tfor short durations the type 'milliseconds' is provided\n"
		          "\tfor very short durations the type 'microseconds' is provided\n"
		          "\tfor time stamps the type 'date' is provided with fractional seconds\n"
		          "\tfor time stamps the type 'idate' is provided with integer seconds\n"
		          "\tshortOpt is the one-letter variant, use '-' to have none\n"
		          "\tlongOpt is the long variant and the name of the shell variable\n"
		          "\tthe rest of the line is the description\n"
		          "\tif 'array' is set the option will be a shell array, which can be\n"
		          "\t  expanded with \"${longOpt[@]}\" which will produce words preserving spaces\n"
		          "\tif 'map' is set the option will be a shell array, which can be\n"
		          "\t  expanded with \"${longOpt[@]}\" which will produce words preserving spaces\n"
		          "\t  other than the 'array' variant maps have string subscripts\n"
		          "\tif 'list' is set the option will be a list, i.e. a variable with the\n"
		          "\t  values separated by spaces, i.e. values may not contain spaces"
		          "\tif 'export' is set the shell variable will be exported\n"
		          "\tif 'positional' is set the variable will be set as postional,\n"
		          "\t  with 'number' defining the order in the positional parameter list.\n"
		          "\tif the next line starts with 'range' the values following are added\n"
		          "\tto the allowed value range of the option, many range lines may follow!\n"
		          "\tif only two are given they denote a true range in the closed interval\n"
		          "\tif the next line starts with 'default' a default value\n"
		          "\t (the rest of line) is set\n"
		          "\tThe keyword 'minusMinusSpecialTreatment' will put the parameters\n"
		          "\tfollowing '--' into the shell variable following that keyword\n"
		          "\tThe keyword 'noPath' clears the search path for config files\n"
		          "\tThe keyword 'path' adds the (escaped) rest of the line\n"
		          "\tto the search path for config files\n"
		          "\tThe keyword 'minUnusedParameters' sets the min number of params\n"
		          "\tThe keyword 'maxUnusedParameters' sets the max number of params\n";

		return (1);
	}

	std::vector<options::base*> options;
	std::set<const options::base*> exportedOptions;
	std::string minusMinusSpecialTreatment = "";
	std::vector<std::string> searchPath({"/etc/", "~/.", "~/.config/", "./."});

	unsigned int minUnusedParameters = 0;
	unsigned int maxUnusedParameters = std::numeric_limits<unsigned int>::max();
	{
		options::parser throwAwayInstance("tAI1");
		std::string keyWord;
		bool exportNextOption = false;
		typeModifierType nextOptionAsWhat = kSimple;
		int nextOptionPositional = 0;
		while (std::cin.good()) {
			std::cin >> keyWord;
			if (std::cin.eof()) {
				break;
			}
			if (keyWord == "string") {
				options.push_back(fOptionFromStream<std::string>(std::cin, "", nextOptionAsWhat));
			} else if (keyWord == "int") {
				options.push_back(fOptionFromStream<int>(std::cin, 0, nextOptionAsWhat));
			} else if (keyWord == "uint") {
				options.push_back(fOptionFromStream<unsigned int>(std::cin, 0, nextOptionAsWhat));
			} else if (keyWord == "bool") {
				options.push_back(fOptionFromStream<bool>(std::cin, false, nextOptionAsWhat));
			} else if (keyWord == "seconds") {
				options.push_back(fOptionFromStream<std::chrono::duration<long long>>(std::cin, std::chrono::seconds(1), nextOptionAsWhat));
			} else if (keyWord == "milliseconds") {
				options.push_back(fOptionFromStream<std::chrono::duration<long long, std::milli>>(std::cin, std::chrono::seconds(1), nextOptionAsWhat));
			} else if (keyWord == "microseconds") {
				options.push_back(fOptionFromStream<std::chrono::duration<long long, std::micro>>(std::cin, std::chrono::seconds(1), nextOptionAsWhat));
			} else if (keyWord == "date") {
				options.push_back(fOptionFromStream<std::chrono::system_clock::time_point>(std::cin, std::chrono::system_clock::now(), nextOptionAsWhat));
			} else if (keyWord == "idate") {
				options.push_back(fOptionFromStream<std::chrono::system_clock::time_point>(std::cin, std::chrono::system_clock::now(), nextOptionAsWhat));
				auto opt = dynamic_cast<options::valuePrinter<std::chrono::system_clock::time_point>*>(options.back());
				if (opt) {
					opt->fSetValuePrinter([](std::ostream & aStream, const std::chrono::system_clock::time_point & aValue)->void {aStream << std::chrono::duration_cast<std::chrono::duration<long>>(aValue.time_since_epoch()).count();});
				}
			} else if (keyWord == "range") {
				options.back()->fAddToRangeFromStream(std::cin);
			} else if (keyWord == "default") {
				options.back()->fAddDefaultFromStream(std::cin);
			} else if (keyWord == "export") {
				exportNextOption = true;
				continue;
			} else if (keyWord == "array") {
				nextOptionAsWhat = kAsArray;
				continue;
			} else if (keyWord == "map") {
				nextOptionAsWhat = kAsMap;
				continue;
			} else if (keyWord == "list") {
				nextOptionAsWhat = kAsList;
				continue;
			} else if (keyWord == "positional") {
				std::cin >> nextOptionPositional;
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
				using options::escapedIO::operator>>;
				std::cin >> buffer;
				searchPath.push_back(buffer);
			} else if (keyWord == "trailer:") {
				break;
			} else {
				std::cerr << "illegal option type '" << keyWord << "', giving up" << std::endl;
				return 1;
			}
			if (nextOptionPositional != 0) {
				options::internal::positional_base(nextOptionPositional, options.back());
			}
			if (exportNextOption) {
				exportedOptions.insert(options.back());
			}
			exportNextOption = false;
			nextOptionAsWhat = kSimple;
			nextOptionPositional = 0;
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

	options::parser parser(description, trailer, searchPath);
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
