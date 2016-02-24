#include "Options.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>


static Option<bool> gOptionDebugOptions('\0', "debugOptions", "give debug output to option parsing");
static Option<bool> gOptionNoCfgFiles('\0', "noCfgFiles", "do not read the default config files, must be FIRST option");
OptionParser* OptionParser::gParser = NULL;

OptionParser::OptionParser(const char *aDescription) {
	if (gParser != NULL) {
		std::cerr << "there may be only one parser" << std::endl;
		exit(1);
	}
	gParser = this;
	lDescription = aDescription;
	lMessageStream = &std::cout;
	lHelpReturnValue = 0;
	fSetAssignmentChars();
}
OptionParser::~OptionParser() {
	gParser = NULL;
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
OptionParser* OptionParser::fGetInstance() {
	return gParser;
}
void OptionParser::fReadConfigFiles() {
	const char *dirs[] = {"/etc/", "~/.", "~/.config/", "./."};
	for (unsigned int i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
		std::string f(dirs[i]);
		auto tildePosition = f.find_first_of('~');
		if (tildePosition != std::string::npos) {
			f.replace(tildePosition, 1, getenv("HOME"));
		}
		f += lProgName;
		fReadCfgFile(f.c_str(), true);
	}
}
const std::vector<std::string>& OptionParser::fParse(int argc, const char *argv[]) {
	lProgName = basename(strdup(argv[0]));
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
					lUnusedOptions.push_back(argv[i]);
				}
				break;
			} else {
				if (NULL == strchr(argv[i], lPrimaryAssignment)) {
					auto it = OptionBase::fGetOptionMap().find(argv[i] + 2);
					if (it == OptionBase::fGetOptionMap().end()) {
						std::cerr << "unknown long option '" << argv[i] << "'"  << std::endl;
						fHelp();
						exit(1);
					}
					auto opt = it->second;
					opt->fHandleOption(argc, argv, &i);
				} else {
					auto buf = strdup(argv[1]);
					auto equalsAt = strchr(buf, lPrimaryAssignment);
					*equalsAt = '\0';
					auto it = OptionBase::fGetOptionMap().find(buf + 2);
					if (it == OptionBase::fGetOptionMap().end()) {
						std::cerr << "unknown long option '" << argv[i] << "'"  << std::endl;
						fHelp();
						exit(1);
					}
					auto opt = it->second;
					opt->fSetMe(equalsAt + 1);
					opt->lSource  = "comdline: ";
					opt->lSource += argv[i];
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
		for (auto it = OptionBase::fGetOptionMap().begin(); it != OptionBase::fGetOptionMap().end(); ++it) {
			auto opt = it->second;
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
		for (auto it = lUnusedOptions.begin(); it != lUnusedOptions.end(); ++it) {
			std::cerr << "unused option '" << *it << "'" << std::endl;
		}
	}
	return lUnusedOptions;
}


void OptionParser::fPrintEscapedString(std::ostream & aStream, const char *aString) {
	bool delimit = strchr(aString, ' ') != NULL || strchr(aString, '\t') != NULL;
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
						*(wp++) = *rp;
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


OptionBase::OptionBase(char aShortName, const std::string & aLongName, const std::string & aExplanation, short aNargs) :
	lShortName(aShortName),
	lLongName(aLongName),
	lExplanation(aExplanation),
	lNargs(aNargs) {
	if (fGetOptionMap().find(lLongName) != fGetOptionMap().end()) {
		throw (lLongName + " already set");
	};
	fGetOptionMap().insert(std::make_pair(lLongName, this));
	if (lShortName != '\0') {
		if (fGetShortOptionMap().find(lShortName) != fGetShortOptionMap().end()) {
			throw (lLongName + " short name already set");
		};
		fGetShortOptionMap().insert(std::make_pair(lShortName, this));
	}
}


void OptionBase::fSetMe(const char *aArg) {
	std::stringstream buf(aArg);
	fSetFromStream(buf);
}
void OptionBase::fHandleOption(int argc, const char *argv[], int *i) {
	if (*i + lNargs >= argc) {
		std::cerr << "option " << lLongName << " needs " << lNargs << " args, but only " << argc - *i - 1 << " remain." << std::endl;
		OptionParser::fGetInstance()->fHelp();
		exit(1);
	}
	if (lNargs == 0) {
		fSetMe(NULL);
	} else if (lNargs == 1) {
		fSetMe(argv[*i + 1]);
		*i += lNargs;
	}
	lSource = "cmdline: ";
	lSource += argv[*i];
	if (fCheckRange(std::cerr) == false) {
		exit(1);
	}
}

void OptionBase::fWriteCfgLines(std::ostream & aStream) const {
	aStream << lLongName << "=";
	fWriteValue(aStream);
}


void OptionParser::fHelp() {
	*lMessageStream << fGetInstance()->lProgName << ": " << fGetInstance()->lDescription << "\n";
	size_t maxName = 0;
	size_t maxExplain = 0;
	for (auto it = OptionBase::fGetOptionMap().begin(); it != OptionBase::fGetOptionMap().end(); ++it) {
		const auto opt = it->second;
		maxName = std::max(opt->lLongName.length(), maxName);
		maxExplain = std::max(opt->lExplanation.length(), maxExplain);
	}
	for (auto it = OptionBase::fGetOptionMap().begin(); it != OptionBase::fGetOptionMap().end(); ++it) {
		const auto opt = it->second;
		if (opt->lShortName != '\0') {
			*lMessageStream << "  -" << opt->lShortName << ", ";
		} else {
			*lMessageStream << "      ";
		}
		*lMessageStream << "--" << std::setw(maxName) << std::left << opt->lLongName
		                << " " << std::setw(maxExplain) << std::left << opt->lExplanation
		                << " default: ";
		opt->fWriteValue(*lMessageStream);
		*lMessageStream << "\n";
	}
	*lMessageStream << std::endl;
}
void OptionParser::fWriteCfgFile(const char *aFileName) {
	std::ofstream cfgFile(aFileName);
	for (auto it = OptionBase::fGetOptionMap().begin(); it != OptionBase::fGetOptionMap().end(); ++it) {
		const auto opt = it->second;
		if (opt->lLongName == "writeCfgFile") {
			continue;
		}
		cfgFile << "\n# " << opt->lExplanation << "\n";
		if (opt->lSource.empty() || opt->lLongName == "readCfgFile") {
			cfgFile << "# ";
		}
		opt->fWriteCfgLines(cfgFile);
		cfgFile << "\n";
		opt->fWriteRange(cfgFile);
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
	while (cfgFile.good()) {
		std::string line;
		std::getline(cfgFile, line);
		lineNumber++;
		if (line[0] == '#' || line.length() == 0) {
			continue;
		}
		auto equalsAt = line.find_first_of('=');
		if (equalsAt == std::string::npos || equalsAt < 1 || equalsAt == line.length()) {
			auto buf = new char[line.length() + 1];
			fReCaptureEscapedString(buf, line.c_str());
			fGetInstance()->lUnusedOptions.push_back(buf);
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
			option->fSetMe(line.substr(equalsAt + 1).c_str());
			option->lSource = aFileName;
			char blubb[128];
			sprintf(blubb, ":%d: ", lineNumber);
			option->lSource += blubb;
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
void Option<bool>::fSetMe(const char *aArg) {
	if (aArg == 0) {
		lValue = ! lValue;
	} else {
		OptionBase::fSetMe(aArg);
	}
}
void Option<bool>::fSetFromStream(std::istream & aStream) {
	aStream >> std::boolalpha >> lValue;
}


Option<const char*>::Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, const char* aDefault, const std::vector<const char *>& aRange) :
	OptionBase(aShortName, aLongName, aExplanation, 1),
	lValue(aDefault) {
	if (!aRange.empty()) {
		fSetRange(aRange);
	}
}


void Option<const char*>::fSetRange(const std::vector<const char *>& aRange) {
	for (auto it = aRange.begin(); it != aRange.end(); ++it) {
		lRange.push_back(strdup(*it));
	}
}
void Option<const char*>::fAddToRangeFromStream(std::istream& aStream) {
	std::string buf1;
	std::getline(aStream, buf1);
	char *buf2 = new char[buf1.length() + 1];
	OptionParser::fReCaptureEscapedString(buf2, buf1.c_str());
	lRange.push_back(buf2);
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
			for (auto it = lRange.begin(); it != lRange.end(); ++it) {
				aStream << "\n# ";
				OptionParser::fPrintEscapedString(aStream, *it);
			}
			aStream << "\n# end of range\n#\n";
		}
	}
}



void Option<const char*>::fWriteValue(std::ostream & aStream) const {
	if (lValue == NULL) {
		aStream << "nullptr";
	} else {
		OptionParser::fPrintEscapedString(aStream, lValue);
	}
}
void Option<const char*>::fSetMe(const char *aArg) {
	char *buf = new char[strlen(aArg + 1)];
	OptionParser::fReCaptureEscapedString(buf, aArg);
	lValue = buf;
}

void Option<const char*>::fSetFromStream(std::istream& /*aStream*/) {
	std::cerr << "must not be called" << std::endl;
	exit(1);
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
		for (auto it = lRange.begin(); it != lRange.end(); ++it) {
			if (strcmp(*it, lValue) == 0) {
				return true;
			}
		}
		aLogStream << fGetLongName() << " out of range (" << lValue << "), must be one of:\n";
		for (auto it = lRange.begin(); it != lRange.end(); ++it) {
			aLogStream << *it << "\n";
		}
		return false;
	}
}




Option<std::string>::Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, const std::string& aDefault, const std::vector<std::string>& aRange) :
	OptionBase(aShortName, aLongName, aExplanation, 1),
	lValue(aDefault) {
	if (!aRange.empty()) {
		fSetRange(aRange);
	}
}

void Option<std::string>::fSetRange( const std::vector<std::string>& aRange) {
	for (auto it = aRange.begin(); it != aRange.end(); ++it) {
		lRange.push_back(*it);
	}
}
void Option<std::string>::fAddToRangeFromStream(std::istream& aStream) {
	std::string buf1;
	std::getline(aStream, buf1);
	char *buf2 = new char[buf1.length() + 1];
	OptionParser::fReCaptureEscapedString(buf2, buf1.c_str());
	lRange.push_back(buf2);
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
		for (auto it = lRange.begin(); it != lRange.end(); ++it) {
			if (*it == lValue) {
				return true;
			}
		}
		aLogStream << fGetLongName() << " out of range (" << lValue << "), must be one of:\n";
		for (auto it = lRange.begin(); it != lRange.end(); ++it) {
			aLogStream << *it << "\n";
		}
		return false;
	}
}


void Option<std::string>::fWriteValue(std::ostream & aStream) const {
	OptionParser::fPrintEscapedString(aStream, lValue.c_str());
}
void Option<std::string>::fSetMe(const char *aArg) {
	char *buf = new char[strlen(aArg + 1)];
	OptionParser::fReCaptureEscapedString(buf, aArg);
	lValue = buf;
	delete[] buf;
}
void Option<std::string>::fSetFromStream(std::istream& /*aStream*/) {
	std::cerr << "must not be called" << std::endl;
	exit(1);
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
			for (auto it = lRange.begin(); it != lRange.end(); ++it) {
				aStream << "\n# ";
				OptionParser::fPrintEscapedString(aStream, it->c_str());
			}
			aStream << "\n# end of range\n#\n";
		}
	}
}

void OptionMap<std::string>::fSetMe(const char *aArg) {
	std::string s(aArg);
	auto dividerPosition = s.find_first_of(OptionParser::fGetInstance()->fGetSecondaryAssignment());
	auto name = s.substr(0, dividerPosition);
	auto buf = new char[s.length() - dividerPosition];
	OptionParser::fReCaptureEscapedString(buf, s.substr(dividerPosition + 1, std::string::npos).c_str());
	(*this)[name] = buf;
}

void OptionMap<std::string>::fWriteCfgLines(std::ostream & aStream) const {
	if (this->empty()) {
		aStream << lLongName << "=key" << OptionParser::fGetInstance()->fGetSecondaryAssignment() << "value\n";
	}
	for (auto it = this->begin(); it != this->end(); ++it) {
		aStream << lLongName << "=" << it->first << OptionParser::fGetInstance()->fGetSecondaryAssignment();
		OptionParser::fPrintEscapedString(aStream, it->second.c_str());
		aStream << "\n";
	}
}
void OptionMap<std::string>::fWriteValue(std::ostream & aStream) const {
	if (this->empty()) {
		aStream << "no value";
	} else {
		for (auto it = this->begin(); it != this->end(); ++it) {
			aStream << it->first << ":" << it->second << " ";
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
	virtual void fSetMe(const char * /*aArg*/) {
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
	virtual void fSetMe(const char *aArg) {
		lValue = aArg;
		OptionParser::fGetInstance()->fWriteCfgFile(aArg);
		exit(0);
	}
};
class OptionReadCfgFile : public Option<const char *> {
  private:
	static OptionReadCfgFile gReadCfgFile;
  public:
	OptionReadCfgFile():
		Option('\0', "readCfgFile", "read a config file") {
	}
	virtual void fSetMe(const char *aArg) {
		lValue = aArg;
		OptionParser::fGetInstance()->fReadCfgFile(aArg);
	}
};


OptionWriteCfgFile OptionWriteCfgFile::gWriteCfgFile;
OptionReadCfgFile OptionReadCfgFile::gReadCfgFile;
OptionHelp OptionHelp::gHelp;
