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
#include <set>

static Option<bool> gOptionDebugOptions('\0', "debugOptions", "give debug output to option parsing");
static Option<bool> gOptionNoCfgFiles('\0', "noCfgFiles", "do not read the default config files, must be FIRST option");
OptionParser* OptionParser::gParser = nullptr;

OptionParser::OptionParser(const char *aDescription, const char *aTrailer, const std::vector<std::string>& aSearchPaths):
	lDescription(aDescription),
	lTrailer(aTrailer),
	lSearchPaths(aSearchPaths) {
	if (gParser != nullptr) {
		std::cerr << "there may be only one parser" << std::endl;
		exit(1);
	}
	gParser = this;
	lMessageStream = &std::cout;
	lHelpReturnValue = 0;
	fSetAssignmentChars();
	lMinusMinusJustEndsOptions = true;
}
OptionParser::~OptionParser() {
	gParser = nullptr;
}
void OptionParser::fSetMessageStream(std::ostream *aStream) {
	lMessageStream = aStream;
}
void OptionParser::fSetHelpReturnValue(int aValue) {
	lHelpReturnValue = aValue;
}
void OptionParser::fSetAssignmentChars(char aPrimary, char aSecondary) {
	lPrimaryAssignment = aPrimary;
	lSecondaryAssignment = aSecondary;
}
void OptionParser::fSetExecutableName(const char *aName) {
	lExecutableName = aName;
}


OptionParser* OptionParser::fGetInstance() {
	return gParser;
}
void OptionParser::fReadConfigFiles() {
	for (auto f : lSearchPaths) {
		auto tildePosition = f.find_first_of('~');
		if (tildePosition != std::string::npos) {
			f.replace(tildePosition, 1, getenv("HOME"));
		}
		f += lProgName;
		fReadCfgFile(f.c_str(), true);
	}
}

const std::vector<std::string>& OptionParser::fParse(int argc, char *argv[]) {
	return fParse(argc, const_cast<const char**>(argv));
}

const std::vector<std::string>& OptionParser::fParse(int argc, const char *argv[]) {
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
				auto it =  OptionBase::fGetShortOptionMap().find(argv[i][j]);
				if (it == OptionBase::fGetShortOptionMap().end()) {
					std::cerr << "unknown short option '" << argv[i][j] << "'"  << std::endl;
					fHelp();
					exit(1);
				} else {
					auto opt = it->second;
					if (opt->lNargs > 0 && strlen(argv[i]) > 2) {
						std::cerr << "run-together short options may not use parameters" << std::endl;
						fHelp();
						exit(1);
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
					auto it = OptionBase::fGetOptionMap().find(argv[i] + 2);
					if (it == OptionBase::fGetOptionMap().end()) {
						std::cerr << "unknown long option '" << argv[i] << "'"  << std::endl;
						fHelp();
						exit(1);
					}
					auto opt = it->second;
					opt->fHandleOption(argc, argv, &i);
				} else {
					auto buf = strdup(argv[i]);
					auto equalsAt = strchr(buf, lPrimaryAssignment);
					*equalsAt = '\0';
					auto it = OptionBase::fGetOptionMap().find(buf + 2);
					if (it == OptionBase::fGetOptionMap().end()) {
						std::cerr << "unknown long option '" << argv[i] << "'"  << std::endl;
						fHelp();
						exit(1);
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
		for (auto & it : OptionBase::fGetOptionMap()) {
			auto opt = it.second;
			std::cerr << "option " << opt->lLongName << " has value '";
			opt->fWriteValue(std::cerr);
			std::cerr << "' ";
			if (opt->lSource.empty()) {
				std::cerr << " (default) ";
			} else {
				std::cerr << " from " << opt->lSource;
			}
			std::cerr << std::endl;
		}
		for (auto & unusedOption : lUnusedOptions) {
			std::cerr << "unused option '" << unusedOption << "'" << std::endl;
		}
	}
	fCheckConsistency();
	return lUnusedOptions;
}

bool OptionParser::fCheckConsistency() {
	std::set<const OptionBase*> optionsThatWereSet;
	for (auto & it : OptionBase::fGetOptionMap()) {
		auto opt = it.second;
		if (opt->fIsSet()) {
			optionsThatWereSet.insert(opt);
		}
	}
	for (auto opt : optionsThatWereSet) {
		for (auto forbidden : opt->lForbiddenOptions) {
			if (optionsThatWereSet.count(forbidden) != 0) {
				std::cerr << "The option " << opt->fGetLongName() << " forbids the use of " << forbidden->fGetLongName() << " but it is given.\n";
				fHelp();
				exit(1);
			}
		}
		for (auto required : opt->lRequiredOptions) {
			if (optionsThatWereSet.count(required) == 0) {
				std::cerr << "The option " << opt->fGetLongName() << " requires the use of " << required->fGetLongName() << " but it is not given.\n";
				fHelp();
				exit(1);
			}
		}
	}
}

void OptionParser::fSetMinusMinusStartsExtraList() {
	lMinusMinusJustEndsOptions = false;
}

void OptionParser::fPrintEscapedString(std::ostream & aStream, const char *aString) {
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

void OptionParser::fReCaptureEscapedString(char *aDest, const char *aSource) {
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


OptionBase::OptionBase(char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs) :
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
OptionBase::~OptionBase() {
	fGetOptionMap().clear();
	fGetShortOptionMap().clear();
	delete lPreserveWorthyStuff;
}

void OptionBase::fSetSource(const char *aSource) {
	if (aSource) {
		lSource = aSource;
	} else {
		lSource.clear();
	}
}


void OptionBase::fHandleOption(int argc, const char *argv[], int *i) {
	if (*i + lNargs >= argc) {
		std::cerr << "option " << lLongName << " needs " << lNargs << " args, but only " << argc - *i - 1 << " remain." << std::endl;
		OptionParser::fGetInstance()->fHelp();
		exit(1);
	}
	if (lNargs == 0) {
		fSetMe(nullptr, "cmdline: ");
	} else if (lNargs == 1) {
		fSetMe(argv[*i + 1], "cmdline: ");
		*i += lNargs;
	}
	if (fCheckRange(std::cerr) == false) {
		exit(1);
	}
}

void OptionBase::fWriteCfgLines(std::ostream & aStream, const char *aPrefix) const {
	aStream << aPrefix << lLongName << "=";
	fWriteValue(aStream);
	aStream << "\n";
}

void OptionBase::fSetPreserveWorthyStuff(std::vector<std::string>* aStuff) {
	lPreserveWorthyStuff = aStuff;
}


void OptionBase::fRequire(const OptionBase* aOtherOption) {
	lRequiredOptions.push_back(aOtherOption);
}
void OptionBase::fRequire(std::vector<const OptionBase*> aOtherOptions) {
	lRequiredOptions.insert(lRequiredOptions.end(), aOtherOptions.cbegin(), aOtherOptions.cend());
}
void OptionBase::fForbid(const OptionBase* aOtherOption) {
	lForbiddenOptions.push_back(aOtherOption);
}
void OptionBase::fForbid(std::vector<const OptionBase*> aOtherOptions) {
	lForbiddenOptions.insert(lForbiddenOptions.end(), aOtherOptions.cbegin(), aOtherOptions.cend());
}


void OptionParser::fPrintOptionHelp(std::ostream& aMessageStream, const OptionBase& aOption, std::size_t aMaxName, std::size_t aMaxExplain, size_t lineLenght) const {
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
}

void OptionParser::fHelp() {
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
	for (auto & it : OptionBase::fGetOptionMap()) {
		const auto opt = it.second;
		maxName = std::max(opt->lLongName.length(), maxName);
		maxExplain = std::max(opt->lExplanation.length(), maxExplain);
	}
	for (auto & it : OptionBase::fGetOptionMap()) {
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
void OptionParser::fWriteCfgFile(const char *aFileName) {
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
	for (auto & it : OptionBase::fGetOptionMap()) {
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

void OptionParser::fReadCfgFile(const char *aFileName, bool aMayBeAbsent) {
	std::ifstream cfgFile(aFileName);
	if (!cfgFile.good() && !aMayBeAbsent) {
		std::cerr << "can't acccess config file '" << aFileName << "', reason is " << strerror(errno) << std::endl;
		exit(1);
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
					auto it = OptionBase::fGetOptionMap().find(optionName);
					if (it != OptionBase::fGetOptionMap().end()) {
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
		auto it = OptionBase::fGetOptionMap().find(optionName);
		if (it == OptionBase::fGetOptionMap().end()) {
			std::cerr << aFileName << ":" << lineNumber << ": error: unknown option '" << optionName << "' ,line is '" << line << "'" << std::endl;
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
			if (option->fCheckRange(std::cerr) == false) {
				exit(1);
			}
		}
	}
	cfgFile.close();
}



void Option<bool>::fWriteValue(std::ostream & aStream) const {
	aStream << std::boolalpha << lValue;
}
void Option<bool>::fSetMe(const char *aArg, const char *aSource) {
	if (aArg == nullptr) {
		lValue = ! lDefault;
	} else {
		std::stringstream buf(aArg);
		buf >> std::boolalpha >> lValue;
	}
	fSetSource(aSource);
}
void Option<bool>::fAddDefaultFromStream(std::istream& aStream) {
	aStream >> std::boolalpha >> lValue;
	lDefault = lValue;
}


Option<const char*>::Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, const char* aDefault, const std::vector<const char *>& aRange) :
	OptionBase(aShortName, aLongName, aExplanation, 1),
	lValue(aDefault) {
	if (!aRange.empty()) {
		fSetRange(aRange);
	}
}

Option<const char*>::~Option() {
	auto i = std::begin(lRange);
	while (i != std::end(lRange)) {
		delete [] *i;
		i = lRange.erase(i);
	}
}

void Option<const char*>::fSetRange(const std::vector<const char *>& aRange) {
	for (auto it : aRange) {
		auto stringCopy = new char[strlen(it) + 1];
		strcpy(stringCopy, it);
		lRange.push_back(stringCopy);
	}
}
void Option<const char*>::fAddToRangeFromStream(std::istream& aStream) {
	std::string buf1;
	std::getline(aStream, buf1);
	auto buf2 = new char[buf1.length() + 1];
	OptionParser::fReCaptureEscapedString(buf2, buf1.c_str());
	lRange.push_back(buf2);
}
void Option<const char*>::fAddDefaultFromStream(std::istream& aStream) {
	std::string buf1;
	std::getline(aStream, buf1);
	fSetMe(buf1.c_str(), nullptr);
}

void  Option<const char*>::fWriteRange(std::ostream &aStream) const {
	if (! lRange.empty()) {
		aStream << "# allowed range is";
		if (lRange.size() == 2) {
			aStream << " [";
			OptionParser::fPrintEscapedString(aStream, lRange[0]);
			aStream << ",";
			OptionParser::fPrintEscapedString(aStream, lRange[1]);
			aStream << "]\n";
		} else {
			aStream << ":";
			for (auto it : lRange) {
				aStream << "\n# ";
				OptionParser::fPrintEscapedString(aStream, it);
			}
			aStream << "\n# end of range\n#\n";
		}
	}
}



void Option<const char*>::fWriteValue(std::ostream & aStream) const {
	if (lValue == nullptr) {
		aStream << "nullptr";
	} else {
		OptionParser::fPrintEscapedString(aStream, lValue);
	}
}
void Option<const char*>::fSetMe(const char *aArg, const char *aSource) {
	auto buf = new char[strlen(aArg) + 1];
	OptionParser::fReCaptureEscapedString(buf, aArg);
	lValue = buf;
	fSetSource(aSource);
}

bool Option<const char*>::fCheckRange(std::ostream& aLogStream) const {
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




Option<std::string>::Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, std::string  aDefault, const std::vector<std::string>& aRange) :
	OptionBase(aShortName, aLongName, aExplanation, 1),
	lValue(std::move(aDefault)) {
	if (!aRange.empty()) {
		fSetRange(aRange);
	}
}

void Option<std::string>::fSetRange( const std::vector<std::string>& aRange) {
	for (const auto & it : aRange) {
		lRange.push_back(it);
	}
}
void Option<std::string>::fAddToRangeFromStream(std::istream& aStream) {
	std::string buf1;
	std::getline(aStream, buf1);
	auto buf2 = new char[buf1.length() + 1];
	OptionParser::fReCaptureEscapedString(buf2, buf1.c_str());
	lRange.push_back(buf2);
}
void Option<std::string>::fAddDefaultFromStream(std::istream& aStream) {
	std::string buf1;
	std::getline(aStream, buf1);
	fSetMe(buf1.c_str(), nullptr);
}

bool Option<std::string>::fCheckRange(std::ostream& aLogStream) const {
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


void Option<std::string>::fWriteValue(std::ostream & aStream) const {
	OptionParser::fPrintEscapedString(aStream, lValue.c_str());
}
void Option<std::string>::fSetMe(const char *aArg, const char *aSource) {
	auto buf = new char[strlen(aArg) + 1];
	OptionParser::fReCaptureEscapedString(buf, aArg);
	lValue = buf;
	delete[] buf;
	fSetSource(aSource);
}

void  Option<std::string>::fWriteRange(std::ostream &aStream) const {
	if (! lRange.empty()) {
		aStream << "# allowed range is";
		if (lRange.size() == 2) {
			aStream << " [";
			OptionParser::fPrintEscapedString(aStream, lRange[0].c_str());
			aStream << ",";
			OptionParser::fPrintEscapedString(aStream, lRange[1].c_str());
			aStream << "]\n";
		} else {
			aStream << ":";
			for (const auto & it : lRange) {
				aStream << "\n# ";
				OptionParser::fPrintEscapedString(aStream, it.c_str());
			}
			aStream << "\n# end of range\n#\n";
		}
	}
}





class OptionHelp : public Option<bool> {
  private:
	static OptionHelp gHelp;
  public:
	OptionHelp():
		Option('h', "help", "give this help") {
	}
	void fSetMe(const char * /*aArg*/, const char * /*aSource*/) override {
		OptionParser::fGetInstance()->fHelp();
		exit(OptionParser::fGetInstance()->fGetHelpReturnValue());
	}
};

class OptionWriteCfgFile : public Option<const char *> {
  private:
	static OptionWriteCfgFile gWriteCfgFile;
  public:
	OptionWriteCfgFile():
		Option('\0', "writeCfgFile", "write a config file") {
	}
	void fSetMe(const char *aArg, const char */* aSource */) override {
		lValue = aArg;
		OptionParser::fGetInstance()->fWriteCfgFile(aArg);
		exit(OptionParser::fGetInstance()->fGetHelpReturnValue());
	}
};
class OptionReadCfgFile : public Option<const char *> {
  private:
	static OptionReadCfgFile gReadCfgFile;
  public:
	OptionReadCfgFile():
		Option('\0', "readCfgFile", "read a config file") {
	}
	void fSetMe(const char *aArg, const char */*aSource*/) override {
		lValue = aArg;
		OptionParser::fGetInstance()->fReadCfgFile(aArg);
	}
};


OptionWriteCfgFile OptionWriteCfgFile::gWriteCfgFile;
OptionReadCfgFile OptionReadCfgFile::gReadCfgFile;
OptionHelp OptionHelp::gHelp;
