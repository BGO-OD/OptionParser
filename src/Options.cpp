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
#include "Options.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctime>
#include <iterator>
#include <algorithm>

namespace options {


	static single<bool> gOptionDebugOptions('\0', "debugOptions", "give debug output to option parsing");
	static single<bool> gOptionNoCfgFiles('\0', "noCfgFiles", "do not read the default config files, must be FIRST option");
	parser* parser::gParser = nullptr;

	parser::parser(const char *aDescription, const char *aTrailer, const std::vector<std::string>& aSearchPaths):
		lDescription(aDescription),
		lTrailer(aTrailer),
		lSearchPaths(aSearchPaths) {
		if (gParser != nullptr) {
			std::cerr << "there may be only one parser" << std::endl;
			fComplainAndLeave(false);
		}
		gParser = this;
		lErrorStream = &std::cerr;
		lMessageStream = &std::cout;
		lHelpReturnValue = 0;
		fSetAssignmentChars();
		lMinusMinusJustEndsOptions = true;
	}
	parser::~parser() {
		gParser = nullptr;
	}
	void parser::fSetMessageStream(std::ostream *aStream) {
		lMessageStream = aStream;
	}
	void parser::fSetErrorStream(std::ostream *aStream) {
		lErrorStream = aStream;
	}
	std::ostream& parser::fGetErrorStream() const {
		return *lErrorStream;
	}
	void parser::fSetHelpReturnValue(int aValue) {
		lHelpReturnValue = aValue;
	}
	void parser::fSetAssignmentChars(char aPrimary, char aSecondary) {
		lPrimaryAssignment = aPrimary;
		lSecondaryAssignment = aSecondary;
	}
	void parser::fSetExecutableName(const char *aName) {
		lExecutableName = aName;
	}

	void parser::fRequire(const base* aOption) {
		lRequiredOptions.insert(aOption);
	}
	void parser::fRequire(std::vector<const base*> aOptions) {
		lRequiredOptions.insert(aOptions.cbegin(), aOptions.cend());
	}



	parser* parser::fGetInstance() {
		return gParser;
	}

/// read config files if present

/// this function iterates over the list of config file search paths,
/// replacing ~ by the home directory and then appends the program name
/// to the path, and if a file is found it is then read as config file.
	void parser::fReadConfigFiles() {
		for (auto f : lSearchPaths) {
			auto tildePosition = f.find_first_of('~');
			if (tildePosition != std::string::npos) {
				f.replace(tildePosition, 1, getenv("HOME"));
			}
			f += lProgName;
			fReadCfgFile(f.c_str(), true);
		}
	}

	const std::vector<std::string>& parser::fParse(int argc, char *argv[]) {
		return fParse(argc, const_cast<const char**>(argv));
	}

	const std::vector<std::string>& parser::fParse(int argc, const char *argv[]) {
		{
			auto buf = strdup(argv[0]);
			lProgName = basename(buf);
			free(buf);
		}
		bool firstOptionNotSeen = true;
		for (int i = 1; i < argc; i++) {
			if (firstOptionNotSeen && !(argv[i][0] == '-' && argv[i][1] == '-' && gOptionNoCfgFiles.lLongName.compare(argv[i] + 2) == 0)) {
				fReadConfigFiles();
			}
			if (argv[i][0] == '-' && argv[i][1] != '-') {
				auto length = strlen(argv[i]);
				for (unsigned int j = 1; j < length; j++) {
					auto it =  base::fGetShortOptionMap().find(argv[i][j]);
					if (it == base::fGetShortOptionMap().end()) {
						fGetErrorStream() << "unknown short option '" << argv[i][j] << "'"  << std::endl;
						fComplainAndLeave();
					} else {
						auto opt = it->second;
						if (opt->lNargs > 0 && strlen(argv[i]) > 2) {
							fGetErrorStream() << "run-together short options may not use parameters" << std::endl;
							fComplainAndLeave();
						}
						opt->fHandleOption(argc, argv, &i);
					}
				}
			} else if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] != '-') {
				auto length = strlen(argv[i]);
				if (length == 2) { // end of options
					for (i++; i < argc; i++) {
						if (lMinusMinusJustEndsOptions) {
							lUnusedOptions.push_back(argv[i]);
						} else {
							lStuffAfterMinusMinus.push_back(argv[i]);
						}
					}
					break;
				} else {
					if (nullptr == strchr(argv[i], lPrimaryAssignment)) {
						auto it = base::fGetOptionMap().find(argv[i] + 2);
						if (it == base::fGetOptionMap().end()) {
							fGetErrorStream() << "unknown long option '" << argv[i] << "'"  << std::endl;
							fComplainAndLeave();
						}
						auto opt = it->second;
						opt->fHandleOption(argc, argv, &i);
					} else {
						auto buf = strdup(argv[i]);
						auto equalsAt = strchr(buf, lPrimaryAssignment);
						*equalsAt = '\0';
						auto it = base::fGetOptionMap().find(buf + 2);
						if (it == base::fGetOptionMap().end()) {
							fGetErrorStream() << "unknown long option '" << argv[i] << "'"  << std::endl;
							fComplainAndLeave();
						}
						auto opt = it->second;
						opt->fSetMe(equalsAt + 1, "cmdline");
						free(buf);
					}
				}
			} else {
				lUnusedOptions.push_back(argv[i]);
			}
			firstOptionNotSeen = false;
		}
		if (firstOptionNotSeen) {
			fReadConfigFiles();
		}
		if (gOptionDebugOptions) {
			for (auto & it : base::fGetOptionMap()) {
				auto opt = it.second;
				fGetErrorStream() << "option " << opt->lLongName << " has value '";
				opt->fWriteValue(fGetErrorStream());
				fGetErrorStream() << "' ";
				if (opt->lSource.empty()) {
					fGetErrorStream() << " (default) ";
				} else {
					fGetErrorStream() << " from " << opt->lSource;
				}
				fGetErrorStream() << std::endl;
			}
			for (auto & unusedOption : lUnusedOptions) {
				fGetErrorStream() << "unused option '" << unusedOption << "'" << std::endl;
			}
		}
		fCheckConsistency();
		return lUnusedOptions;
	}

	void parser::fCheckConsistency() {
		std::set<const base*> optionsThatWereSet;
		for (auto & it : base::fGetOptionMap()) {
			auto opt = it.second;
			if (opt->fIsSet()) {
				optionsThatWereSet.insert(opt);
			}
		}
		if (! lRequiredOptions.empty()) {
			std::vector<const base*> missing;
			std::set_difference(lRequiredOptions.cbegin(), lRequiredOptions.cend(),
			                    optionsThatWereSet.cbegin(), optionsThatWereSet.cend(),
			                    std::inserter(missing, missing.begin()));
			if (! missing.empty()) {
				fGetErrorStream() << "The following options are required but were not given:";
				for (auto opt : missing) {
					fGetErrorStream() << " " << opt->fGetLongName();
				}
				fGetErrorStream() << "\n";
				fComplainAndLeave();
			}
		}
		for (auto opt : optionsThatWereSet) {
			for (auto forbidden : opt->lForbiddenOptions) {
				if (optionsThatWereSet.count(forbidden) != 0) {
					fGetErrorStream() << "The option " << opt->fGetLongName() << " forbids the use of " << forbidden->fGetLongName() << " but it is given.\n";
					fComplainAndLeave();
				}
			}
			for (auto required : opt->lRequiredOptions) {
				if (optionsThatWereSet.count(required) == 0) {
					fGetErrorStream() << "The option " << opt->fGetLongName() << " requires the use of " << required->fGetLongName() << " but it is not given.\n";
					fComplainAndLeave();
				}
			}
		}
	}

/// print help (if required) and exit.

/// this function can be overloaded in a class derived from parser, for example when one instead of calling exit() wants to throw an exception.
/// as normaly the call to exit() is what is required this is the default action.
	void parser::fComplainAndLeave(bool aWithHelp) {
		if (aWithHelp) {
			fHelp();
		}
		exit(1);
	}

	void parser::fSetMinusMinusStartsExtraList() {
		lMinusMinusJustEndsOptions = false;
	}

	void parser::fPrintEscapedString(std::ostream & aStream, const char *aString) {
		bool delimit = (strchr(aString, ' ') != nullptr)
		               || (strchr(aString, '\t') != nullptr)
		               || (strchr(aString, ',') != nullptr);
		if (delimit) {
			aStream << '"';
		}
		for (const char *rp = aString; *rp != '\0'; rp++) {
			switch (*rp) {
				case '\a':
					aStream << "\\a";
					break;
				case '\b':
					aStream << "\\b";
					break;
				case '\f':
					aStream << "\\f";
					break;
				case '\n':
					aStream << "\\n";
					break;
				case '\r':
					aStream << "\\r";
					break;
				case '\t':
					aStream << "\\t";
					break;
				case '\v':
					aStream << "\\v";
					break;
				case '\\':
					aStream << "\\\\";
					break;
				case '\'':
					aStream << "\\\'";
					break;
				case '"':
					aStream << "\\\"";
					break;
				default:
					if (*rp >= ' ' && *rp < 127) {
						aStream << *rp;
					} else {
						aStream << '\\' << std::oct << std::setw(3) << std::setfill('0') << static_cast<unsigned int>(*rp) << std::setfill(' ') << std::dec;
					}
			}
		}
		if (delimit) {
			aStream << '"';
		}
	}

	void parser::fReCaptureEscapedString(char *aDest, const char *aSource) {
		auto wp = aDest;
		auto rp = aSource;
		auto delimiter = *aSource;
		if (delimiter == '"' || delimiter == '\'') { // string is enclosed in a pair of delimiters
			rp++;
		} else { // skip whitespace
			delimiter = '\0';
			rp += strspn(rp, " \t");
		}
		for (; *rp;) {
			if (*rp == '\\') {
				rp++;
				switch (*rp) {
					case 'a':
						*(wp++) = '\a';
						rp++;
						break;
					case 'b':
						*(wp++) = '\b';
						rp++;
						break;
					case 'f':
						*(wp++) = '\f';
						rp++;
						break;
					case 'n':
						*(wp++) = '\n';
						rp++;
						break;
					case 'r':
						*(wp++) = '\r';
						rp++;
						break;
					case 't':
						*(wp++) = '\t';
						rp++;
						break;
					case 'v':
						*(wp++) = '\v';
						rp++;
						break;
					default:
						if (*rp >= '0' && *rp <= '7') {
							char c = 0;
							for (int i = 0; i < 3 && *rp >= '0' && *rp <= '7'; i++) {
								c = (c << 3) | ((*(rp++) - '0') & 0x7);
							}
							*(wp++) = c;
						} else {
							*(wp++) = *(rp++);
						}
						break;
				}
			} else if (*rp == delimiter) {
				break;
			} else {
				*(wp++) = *(rp++);
			}
		}
		*wp = '\0';
	}

/// construct an opbect of type base

/// The newluy created object is inserted into the maps sorted by long name and by short name,
/// (short name only if it is not '\0')
/// If a clash wopuld occur, i.e. either long or short name is already taken an exception is thrown.
/// \param [in] aShortName short option without the -, use '\0' to have only a long form
/// \param [in] aLongName long option without the --, must always be given
/// \param [in] aExplanation explanation for help output
/// \param [in] aNargs number of arguments/parameters. May be 0 ot 1.
	base::base(char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs) :
		lShortName(aShortName),
		lLongName(aLongName),
		lExplanation(aExplanation),
		lNargs(aNargs) {
		if (fGetOptionMap().find(lLongName) != fGetOptionMap().end()) {
			std::string exceptionText(lLongName + " already set");
			throw std::invalid_argument(exceptionText);
		};
		fGetOptionMap().insert(std::make_pair(lLongName, this));
		if (lShortName != '\0') {
			if (fGetShortOptionMap().find(lShortName) != fGetShortOptionMap().end()) {
				std::string exceptionText(lLongName + ": short name '" + lShortName + "' already set");
				throw std::invalid_argument(exceptionText);
			};
			fGetShortOptionMap().insert(std::make_pair(lShortName, this));
		}
		lPreserveWorthyStuff = nullptr;
	}
	base::~base() {
		fGetOptionMap().clear();
		fGetShortOptionMap().clear();
		delete lPreserveWorthyStuff;
	}

	void base::fSetSource(const char *aSource) {
		if (aSource) {
			lSource = aSource;
		} else {
			lSource.clear();
		}
	}


	void base::fHandleOption(int argc, const char *argv[], int *i) {
		if (*i + lNargs >= argc) {
			parser::fGetInstance()->fGetErrorStream() << "option " << lLongName << " needs " << lNargs << " args, but only " << argc - *i - 1 << " remain." << std::endl;
			parser::fGetInstance()->fComplainAndLeave();
		}
		if (lNargs == 0) {
			fSetMe(nullptr, "cmdline: ");
		} else if (lNargs == 1) {
			fSetMe(argv[*i + 1], "cmdline: ");
			*i += lNargs;
		}
		if (fCheckRange(parser::fGetInstance()->fGetErrorStream()) == false) {
			parser::fGetInstance()->fComplainAndLeave(false);
		}
	}

	void base::fWriteCfgLines(std::ostream & aStream, const char *aPrefix) const {
		aStream << aPrefix << lLongName << "=";
		fWriteValue(aStream);
		aStream << "\n";
	}

	void base::fSetPreserveWorthyStuff(std::vector<std::string>* aStuff) {
		lPreserveWorthyStuff = aStuff;
	}


	void base::fRequire(const base* aOtherOption) {
		lRequiredOptions.push_back(aOtherOption);
	}
	void base::fRequire(std::vector<const base*> aOtherOptions) {
		lRequiredOptions.insert(lRequiredOptions.end(), aOtherOptions.cbegin(), aOtherOptions.cend());
	}
	void base::fForbid(const base* aOtherOption) {
		lForbiddenOptions.push_back(aOtherOption);
	}
	void base::fForbid(std::vector<const base*> aOtherOptions) {
		lForbiddenOptions.insert(lForbiddenOptions.end(), aOtherOptions.cbegin(), aOtherOptions.cend());
	}


	void parser::fPrintOptionHelp(std::ostream& aMessageStream, const base& aOption, std::size_t aMaxName, std::size_t aMaxExplain, size_t lineLenght) const {
		std::size_t fullNameLength = 8 + aMaxName;
		std::size_t descLength = std::min(lineLenght - 20 - fullNameLength, aMaxExplain);
		if (aOption.lShortName != '\0') {
			aMessageStream << "  -" << aOption.lShortName << ", ";
		} else {
			aMessageStream << "      ";
		}
		auto explanation = aOption.lExplanation;
		bool firstLine = true;
		do {
			if (firstLine) {
				aMessageStream << "--" << std::setw(aMaxName) << std::left << aOption.lLongName
				               << " ";
			} else {
				aMessageStream << "        " << std::setw(aMaxName) << " " << "  ";
			}
			if (explanation.length() > descLength) {
				// Need to linebreak.
				bool keepBrokenChar = false;
				bool hardBreak = false;
				auto breakPos = explanation.find_last_of(" -", descLength - 1);
				if (breakPos == std::string::npos) {
					// No space, use hard break. In this case, keep the broken character.
					// -2 since we add a "-" separator for readability.
					breakPos = descLength - 2;
					hardBreak = true;
					keepBrokenChar = true;
				} else {
					// We also want to keep the broken character in case it's not a whitespace.
					if (explanation[breakPos] != ' ') {
						breakPos++;
						keepBrokenChar = true;
					}
				}
				aMessageStream << std::setw(descLength) << std::left << explanation.substr(0, breakPos) + (hardBreak ? "-" : "");
				explanation = explanation.substr(breakPos + (keepBrokenChar ? 0 : 1));
			} else {
				aMessageStream << std::setw(descLength) << std::left << explanation;
				// Trigger delayed break.
				explanation.clear();
			}
			if (firstLine) {
				aMessageStream << " default: ";
				aOption.fWriteValue(aMessageStream);
				firstLine = false;
			}
			aMessageStream << "\n";
		} while (explanation.length() > 0);
		if (! aOption.lRequiredOptions.empty()) {
			aMessageStream << "      " << std::setw(aMaxName) << " " << " Requires the following other options:\n";
			for (auto opt : aOption.lRequiredOptions) {
				aMessageStream << "      " << std::setw(aMaxName) << " " << "  " << opt->fGetLongName() << "\n";
			}
		}
		if (! aOption.lForbiddenOptions.empty()) {
			aMessageStream << "      " << std::setw(aMaxName) << " " << " Forbids the following other options:\n";
			for (auto opt : aOption.lForbiddenOptions) {
				aMessageStream << "      " << std::setw(aMaxName) << " " << "  " << opt->fGetLongName() << "\n";
			}
		}
		if (lRequiredOptions.count(&aOption)) {
			aMessageStream << "      " << std::setw(aMaxName) << " " << "   this option is required\n";
		}
	}

	void parser::fHelp() {
		*lMessageStream << lProgName << ": " << lDescription << "\n";
		size_t maxName = 0;
		size_t maxExplain = 0;
		size_t lineLenght = 132;
		{
			struct winsize window;
			if (ioctl(0, TIOCGWINSZ, &window) == 0) {
				lineLenght = window.ws_col;
			}
		}
		for (auto & it : base::fGetOptionMap()) {
			const auto opt = it.second;
			maxName = std::max(opt->lLongName.length(), maxName);
			maxExplain = std::max(opt->lExplanation.length(), maxExplain);
		}
		for (auto & it : base::fGetOptionMap()) {
			const auto opt = it.second;
			fPrintOptionHelp(*lMessageStream, *opt, maxName, maxExplain, lineLenght);
		}
		if (!lSearchPaths.empty()) {
			*lMessageStream << "Looking for config files in ";
			for (const auto & searchPath : lSearchPaths) {
				*lMessageStream << searchPath << " ";
			}
			*lMessageStream << "\n";
		}

		if (lTrailer != nullptr) {
			*lMessageStream << lTrailer;
		}
		*lMessageStream << std::endl;
	}
	void parser::fWriteCfgFile(const char *aFileName) {
		std::ofstream cfgFile(aFileName, std::ofstream::out | std::ofstream::trunc);
		if (lExecutableName.empty()) {
			char buf[128];
			auto result = readlink("/proc/self/exe", buf, sizeof(buf));
			if (result > 0 && result < 128 - 2 - 14) {
				cfgFile << "#!" << buf << " --readCfgFile\n";
			}
		} else {
			cfgFile << "#!" << lExecutableName << " --readCfgFile\n";
		}
		{
			cfgFile << "# written ";
			std::time_t t = std::time(nullptr);
			char mbstr[100];
			if (std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %H:%M:%S", std::localtime(&t))) {
				cfgFile << mbstr;
			}
			cfgFile << " by " << getenv("USER") << " using " << lProgName << "\n";
			cfgFile << "# only comments started by ## will be preserved on a re-write!\n";
			cfgFile << "# to rewrite this file for a different executable location, use:\n";
			cfgFile << "# " << lProgName << " --readCfgFile " << aFileName << " --writeCfgFile " << aFileName << "\n";
			cfgFile << "# Assuming " << lProgName << " is in your PATH\n";
		}
		for (auto & it : base::fGetOptionMap()) {
			const auto opt = it.second;
			if (opt->lLongName == "writeCfgFile") {
				continue;
			}
			if (opt->lPreserveWorthyStuff != nullptr) {
				for (const auto& line : * (opt->lPreserveWorthyStuff)) {
					cfgFile << "\n" << line;
				}
			}
			cfgFile << "\n# " << opt->lExplanation << "\n";
			auto prefix = "";
			if (opt->lSource.empty() || opt->lLongName == "readCfgFile") {
				prefix = "# ";
			}
			opt->fWriteCfgLines(cfgFile, prefix);
			opt->fWriteRange(cfgFile);
			if (!opt->lSource.empty()) {
				cfgFile << "# set from " << opt->lSource << "\n";
			}
		}
		cfgFile << "#\n";
		cfgFile.close();
	}

	void parser::fReadCfgFile(const char *aFileName, bool aMayBeAbsent) {
		std::ifstream cfgFile(aFileName);
		if (!cfgFile.good() && !aMayBeAbsent) {
			fGetErrorStream() << "can't acccess config file '" << aFileName << "', reason is " << strerror(errno) << std::endl;
			fComplainAndLeave(false);
		}
		int lineNumber = 0;
		std::vector<std::string>* preserveWorthyStuff = nullptr;
		while (cfgFile.good()) {
			std::string line;
			std::getline(cfgFile, line);
			lineNumber++;
			if (line.length() == 0) {
				continue;
			} else if (line[0] == '#') {
				if (line[1] == '#') {
					if (preserveWorthyStuff == nullptr) {
						preserveWorthyStuff = new std::vector<std::string>;
					}
					preserveWorthyStuff->push_back(line);
				} else if (preserveWorthyStuff != nullptr) {
					auto equalsAt = line.find_first_of('=');
					if (equalsAt != std::string::npos) {
						auto optionName = line.substr(2, equalsAt - 2);
						auto it = base::fGetOptionMap().find(optionName);
						if (it != base::fGetOptionMap().end()) {
							auto option = it->second;
							option->fSetPreserveWorthyStuff(preserveWorthyStuff);
							preserveWorthyStuff = nullptr;
						}
					}
				}
				continue;
			}
			auto equalsAt = line.find_first_of('=');
			if (equalsAt == std::string::npos || equalsAt < 1 || equalsAt == line.length()) {
				auto buf = new char[line.length() + 1];
				fReCaptureEscapedString(buf, line.c_str());
				lUnusedOptions.push_back(buf);
				delete [] buf;
				continue;
			}
			auto optionName = line.substr(0, equalsAt);
			auto it = base::fGetOptionMap().find(optionName);
			if (it == base::fGetOptionMap().end()) {
				fGetErrorStream() << aFileName << ":" << lineNumber << ": error: unknown option '" << optionName << "' ,line is '" << line << "'" << std::endl;
				continue;
			}
			auto option = it->second;
			{
				char blubb[1028];
				sprintf(blubb, "%s:%d: ", aFileName, lineNumber);
				option->fSetMe(line.substr(equalsAt + 1).c_str(), blubb);
				if (preserveWorthyStuff != nullptr) {
					option->fSetPreserveWorthyStuff(preserveWorthyStuff);
					preserveWorthyStuff = nullptr;
				}
				if (option->fCheckRange(fGetErrorStream()) == false) {
					fComplainAndLeave();
				}
			}
		}
		cfgFile.close();
	}


/// \class  single<bool>
/// in this class in cases that there is no argument, i.e. when parsing the comand line
/// the value is set to the oppsoite of the default value.
/// If an argument is given, which is always the case in config files,
/// then the value is read as std::boolalpha (when no locale nonsense is done 'true' or 'false')


	void single<bool>::fWriteValue(std::ostream & aStream) const {
		aStream << std::boolalpha << lValue;
	}
	void single<bool>::fSetMe(const char *aArg, const char *aSource) {
		if (aArg == nullptr) {
			lValue = ! lDefault;
		} else {
			std::stringstream buf(aArg);
			buf >> std::boolalpha >> lValue;
		}
		fSetSource(aSource);
	}
	void single<bool>::fAddDefaultFromStream(std::istream& aStream) {
		aStream >> std::boolalpha >> lValue;
		lDefault = lValue;
	}

/// \copydetails Option::Option()
	single<const char*>::single(char aShortName, const std::string& aLongName, const std::string& aExplanation, const char* aDefault, const std::vector<const char *>& aRange) :
		base(aShortName, aLongName, aExplanation, 1),
		lValue(aDefault) {
		if (!aRange.empty()) {
			fAddToRange(aRange);
		}
	}

	single<const char*>::~single() {
		auto i = std::begin(lRange);
		while (i != std::end(lRange)) {
			delete [] *i;
			i = lRange.erase(i);
		}
	}
/// add a copy of the given string to the range of allowe values
	void single<const char*>::fAddToRange(const char *aValue) {
		auto size = strlen(aValue) + 1;
		auto stringCopy = new char[size];
		strncpy(stringCopy, aValue, size);
		lRange.push_back(stringCopy);
	}
/// \copydoc Option::fAddToRange(const std::vector< T > &)
	void single<const char*>::fAddToRange(const std::vector<const char *>& aRange) {
		fAddToRange(aRange.cbegin(), aRange.cend());
	}
/// \details read a line from aStream and undo any escapes found using 	parser::fReCaptureEscapedString()
	void single<const char*>::fAddToRangeFromStream(std::istream& aStream) {
		std::string buf1;
		std::getline(aStream, buf1);
		auto buf2 = new char[buf1.length() + 1];
		parser::fReCaptureEscapedString(buf2, buf1.c_str());
		lRange.push_back(buf2);
	}
	void single<const char*>::fAddDefaultFromStream(std::istream& aStream) {
		std::string buf1;
		std::getline(aStream, buf1);
		fSetMe(buf1.c_str(), nullptr);
	}

	void  single<const char*>::fWriteRange(std::ostream &aStream) const {
		if (! lRange.empty()) {
			aStream << "# allowed range is";
			if (lRange.size() == 2) {
				aStream << " [";
				parser::fPrintEscapedString(aStream, lRange[0]);
				aStream << ",";
				parser::fPrintEscapedString(aStream, lRange[1]);
				aStream << "]\n";
			} else {
				aStream << ":";
				for (auto it : lRange) {
					aStream << "\n# ";
					parser::fPrintEscapedString(aStream, it);
				}
				aStream << "\n# end of range\n#\n";
			}
		}
	}



	void single<const char*>::fWriteValue(std::ostream & aStream) const {
		if (lValue == nullptr) {
			aStream << "nullptr";
		} else {
			parser::fPrintEscapedString(aStream, lValue);
		}
	}
	void single<const char*>::fSetMe(const char *aArg, const char *aSource) {
		auto buf = new char[strlen(aArg) + 1];
		parser::fReCaptureEscapedString(buf, aArg);
		lValue = buf;
		fSetSource(aSource);
	}

	bool single<const char*>::fCheckRange(std::ostream& aLogStream) const {
		if (lRange.empty()) {
			return true;
		} else if (lRange.size() == 2) {
			if (strcmp(lRange[0], lValue) < 1 && strcmp(lValue, lRange[1]) < 1) {
				return true;
			} else {
				aLogStream << fGetLongName() << " out of range (" << lValue << "), must be in [" << lRange[0] << ", " << lRange[1] << "]\n";
				return false;
			}
		} else {
			for (auto it : lRange) {
				if (strcmp(it, lValue) == 0) {
					return true;
				}
			}
			aLogStream << fGetLongName() << " out of range (" << lValue << "), must be one of:\n";
			for (auto it : lRange) {
				aLogStream << it << "\n";
			}
			return false;
		}
	}




	single<std::string>::single(char aShortName, const std::string& aLongName, const std::string& aExplanation, std::string  aDefault, const std::vector<std::string>& aRange) :
		base(aShortName, aLongName, aExplanation, 1),
		lValue(std::move(aDefault)) {
		if (!aRange.empty()) {
			fAddToRange(aRange);
		}
	}
	void single<std::string>::fAddToRange( const std::string& aValue) {
		lRange.push_back(aValue);
	}
	void single<std::string>::fAddToRange( const std::vector<std::string>& aRange) {
		fAddToRange(aRange.cbegin(), aRange.cend());
	}
	void single<std::string>::fAddToRangeFromStream(std::istream& aStream) {
		std::string buf1;
		std::getline(aStream, buf1);
		auto buf2 = new char[buf1.length() + 1];
		parser::fReCaptureEscapedString(buf2, buf1.c_str());
		lRange.push_back(buf2);
	}
	void single<std::string>::fAddDefaultFromStream(std::istream& aStream) {
		std::string buf1;
		std::getline(aStream, buf1);
		fSetMe(buf1.c_str(), nullptr);
	}

	bool single<std::string>::fCheckRange(std::ostream& aLogStream) const {
		if (lRange.empty()) {
			return true;
		} else if (lRange.size() == 2) {
			if (lRange[0].compare(lValue) < 1 && lValue.compare(lRange[1]) < 1) {
				return true;
			} else {
				aLogStream << fGetLongName() << " out of range (" << lValue << "), must be in [" << lRange[0] << ", " << lRange[1] << "]\n";
				return false;
			}
		} else {
			for (const auto & it : lRange) {
				if (it == lValue) {
					return true;
				}
			}
			aLogStream << fGetLongName() << " out of range (" << lValue << "), must be one of:\n";
			for (const auto & it : lRange) {
				aLogStream << it << "\n";
			}
			return false;
		}
	}


	void single<std::string>::fWriteValue(std::ostream & aStream) const {
		parser::fPrintEscapedString(aStream, lValue.c_str());
	}
	void single<std::string>::fSetMe(const char *aArg, const char *aSource) {
		auto buf = new char[strlen(aArg) + 1];
		parser::fReCaptureEscapedString(buf, aArg);
		lValue = buf;
		delete[] buf;
		fSetSource(aSource);
	}

	void  single<std::string>::fWriteRange(std::ostream &aStream) const {
		if (! lRange.empty()) {
			aStream << "# allowed range is";
			if (lRange.size() == 2) {
				aStream << " [";
				parser::fPrintEscapedString(aStream, lRange[0].c_str());
				aStream << ",";
				parser::fPrintEscapedString(aStream, lRange[1].c_str());
				aStream << "]\n";
			} else {
				aStream << ":";
				for (const auto & it : lRange) {
					aStream << "\n# ";
					parser::fPrintEscapedString(aStream, it.c_str());
				}
				aStream << "\n# end of range\n#\n";
			}
		}
	}




/// special derived class used to give help
	class OptionHelp : public single<bool> {
	  private:
		static OptionHelp gHelp;
	  public:
		OptionHelp():
			single('h', "help", "give this help") {
		}
		void fSetMe(const char * /*aArg*/, const char * /*aSource*/) override {
			parser::fGetInstance()->fHelp();
			exit(parser::fGetInstance()->fGetHelpReturnValue());
		}
	};

/// special derived class used to write out config files
	class OptionWriteCfgFile : public single<const char *> {
	  private:
		static OptionWriteCfgFile gWriteCfgFile;
	  public:
		OptionWriteCfgFile():
			single('\0', "writeCfgFile", "write a config file") {
		}
		void fSetMe(const char *aArg, const char */* aSource */) override {
			lValue = aArg;
			parser::fGetInstance()->fWriteCfgFile(aArg);
			exit(parser::fGetInstance()->fGetHelpReturnValue());
		}
	};

/// special derived class used to read in config files
	class OptionReadCfgFile : public single<const char *> {
	  private:
		static OptionReadCfgFile gReadCfgFile;
	  public:
		OptionReadCfgFile():
			single('\0', "readCfgFile", "read a config file") {
		}
		void fSetMe(const char *aArg, const char */*aSource*/) override {
			lValue = aArg;
			parser::fGetInstance()->fReadCfgFile(aArg);
		}
	};


	OptionWriteCfgFile OptionWriteCfgFile::gWriteCfgFile;
	OptionReadCfgFile OptionReadCfgFile::gReadCfgFile;
	OptionHelp OptionHelp::gHelp;

} // end of namespace options
