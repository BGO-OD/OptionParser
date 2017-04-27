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
#include <sys/stat.h>
#include <sys/types.h>
#ifdef IS_NONBROKEN_SYSTEM
#include <termios.h>
#include <sys/ioctl.h>
#endif
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctime>
#include <iterator>
#include <algorithm>
#include <list>

/// \namespace options
/// all of the option parser stuff is contained in the namespace options.

namespace options {
	namespace internal {
		// ugly trick here: we give a reference to the newly construcetd item to the constructor
		// we have only one constructor that needs the reference as parameter
		const sourceFile sourceFile::gUnsetSource("unset",   sourceFile::gUnsetSource);
		const sourceFile sourceFile::gCmdLine("commandLine", sourceFile::gUnsetSource);
	} // end of namespace internal


	/// operator to write sourceItems to a stream
	std::ostream& operator<< (std::ostream &aStream, const internal::sourceItem& aItem) {
		aStream << aItem.fGetFile()->fGetName() << ":" << aItem.fGetLineNumber();
		return aStream;
	}

	namespace internal { // for clarity we put these options into the
		// internal namespace, although they are already hidden by the static
		/// standard option for producing debug output about the options
		static single<bool> gOptionDebugOptions('\0', "debugOptions", "give debug output to option parsing");
		/// standard option to suppress parsing of config files
		static single<bool> gOptionNoCfgFiles('\0', "noCfgFiles", "do not read the default config files, must be FIRST option");


		positional_base::positional_base(int aOrderingNumber,
		                                 base* aAsBase) {
			if (aAsBase->fIsContainer()) {
				for (const auto& it : fGetPositonalArgs()) {
					if (it.second->fIsContainer()) {
						throw std::logic_error("only one container type option allowed");
					}
				}
			}
			auto result = fGetPositonalArgs().emplace(aOrderingNumber, aAsBase);
			if (result.second == false) {
				throw std::logic_error("non-unique numbered positional arg");
			}
		};


	} // end of namespace internal

	parser* parser::gParser = nullptr;

	parser::parser(const std::string& aDescription, const std::string& aTrailer, const std::vector<std::string>& aSearchPaths):
		lDescription(aDescription),
		lTrailer(aTrailer),
		lSearchPaths(aSearchPaths),
		lParsingIsDone(false) {
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

	bool parser::fIsParsingDone() const {
		return lParsingIsDone;
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
			fReadCfgFile(f, internal::sourceItem(), true);
		}
	}

	const std::vector<std::string>& parser::fParse(int argc, char *argv[]) {
		return fParse(argc, const_cast<const char**>(argv));
	}

	const std::vector<std::string>& parser::fParse(int argc, const char *argv[]) {
		if (lParsingIsDone) {
			throw std::logic_error("parsing may be done only once");
		}
		lParsingIsDone = true; // we set this early, as of now now new options may be created
		{
			#ifdef IS_NONBROKEN_SYSTEM
			auto buf = strdup(argv[0]);
			lProgName = basename(buf);
			free(buf);
			#else
			lProgName = argv[0];
			#endif
		}
		bool firstOptionNotSeen = true;
		for (int i = 1; i < argc; i++) {
			if (firstOptionNotSeen && !(argv[i][0] == '-' && argv[i][1] == '-' && internal::gOptionNoCfgFiles.lLongName.compare(argv[i] + 2) == 0)) {
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
						std::stringstream sbuf(equalsAt + 1);
						opt->fSetMe(sbuf, internal::sourceItem(&internal::sourceFile::gCmdLine, i));
						if (!opt->fCheckRange(fGetErrorStream())) {
							fComplainAndLeave(false);
						}
						free(buf);
					}
				}
			} else {
				lUnusedOptions.push_back(argv[i]);
			}
			firstOptionNotSeen = false;
		}
		if (firstOptionNotSeen) { // read cfg files if no options set at all
			fReadConfigFiles();
		}

		if (!internal::positional_base::fGetPositonalArgs().empty()) {
			std::list<base*> positionalArgs;
			for (auto it : internal::positional_base::fGetPositonalArgs()) {
				positionalArgs.push_back(it.second);
			}
			while (!lUnusedOptions.empty()) {
				auto opt = positionalArgs.front();
				if (opt->fIsContainer()) { // process first options from the back
					for (;;) {
						auto opt2 = positionalArgs.back();
						if (opt2 == opt) {
							break;
						}
						positionalArgs.pop_back();
						auto& arg = lUnusedOptions.back();
						std::stringstream sbuf(arg);
						opt2->fSetMe(sbuf, internal::sourceItem(&internal::sourceFile::gCmdLine, 0));
						if (!opt2->fCheckRange(fGetErrorStream())) {
							fComplainAndLeave(false);
						}
						lUnusedOptions.pop_back();
						if (lUnusedOptions.empty()) {
							break;
						}
					}
				}
				auto& arg = lUnusedOptions.front();
				std::stringstream sbuf(arg);
				opt->fSetMe(sbuf, internal::sourceItem(&internal::sourceFile::gCmdLine, 0));
				if (!opt->fCheckRange(fGetErrorStream())) {
					fComplainAndLeave(false);
				}
				lUnusedOptions.erase(lUnusedOptions.begin());
				if (! opt->fIsContainer()) {
					positionalArgs.pop_front();
					if (positionalArgs.empty()) {
						break;
					}
				}
			}
		}


		if (internal::gOptionDebugOptions) {
			for (auto & it : base::fGetOptionMap()) {
				auto opt = it.second;
				fGetErrorStream() << "option " << opt->lLongName << " has value '";
				opt->fWriteValue(fGetErrorStream());
				fGetErrorStream() << "' ";
				if (opt->lSource.fIsUnset()) {
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

	void parser::fPrintEscapedString(std::ostream & aStream, const std::string& aString) {
		bool delimit = aString.find_first_of(" \t,") != std::string::npos;
		if (delimit) {
			aStream << '\'';
		}
		for (auto c : aString) {
			switch (c) {
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
				case ' ':
					aStream << "\\ ";
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
					if (c >= ' ' && c < 127) {
						aStream << c;
					} else {
						aStream << '\\' << std::oct << std::setw(3) << std::setfill('0') << static_cast<unsigned int>(c) << std::setfill(' ') << std::dec;
					}
			}
		}
		if (delimit) {
			aStream << '\'';
		}
	}

	void parser::fReCaptureEscapedString(std::string& aDest, const std::string& aSource) {
		std::stringstream buffer(aSource);
		buffer >> aDest;
	}
	namespace escapedIO {

		std::istream& operator>> (std::istream &aStream, const char*& aCstring) {
			auto buffer = new std::string;
			aStream >> *buffer;
			if (aStream.fail()) {
				delete buffer;
			} else { // leak a string...
				aCstring = buffer->c_str();
			}
			return aStream;
		}


		std::ostream& operator<<(std::ostream& aStream, const std::string& aString) {
			parser::fPrintEscapedString(aStream, aString);
			return aStream;
		}
		std::istream& operator>>(std::istream& aStream, std::string& aString) {
			std::istream::sentry s(aStream);
			if (s) {
				aString.clear();
				auto delimiter = aStream.peek();
				if (delimiter == '"' || delimiter == '\'') { // string is enclosed in a pair of delimiters
					aStream.get();
				} else {
					delimiter = '\0';
				}
				while (! aStream.eof()) {
					auto c = aStream.peek();
					if (c == '\n' || aStream.eof()) {
						break;
					}
					aStream.get();
					if (c == '\\') {
						c = aStream.get();
						switch (c) {
							case 'a':
								aString.push_back('\a');
								break;
							case 'b':
								aString.push_back('\b');
								break;
							case 'f':
								aString.push_back('\f');
								break;
							case 'n':
								aString.push_back('\n');
								break;
							case 'r':
								aString.push_back('\r');
								break;
							case 't':
								aString.push_back('\t');
								break;
							case 'v':
								aString.push_back('\v');
								break;
							default:
								if (c >= '0' && c <= '7') {
									char ch = 0;
									for (int i = 0; i < 3 && c >= '0' && c <= '7'; i++) {
										ch = (ch << 3) | ((c - '0') & 0x7);
										c = aStream.get();
									}
									aString.push_back(ch);
								} else {
									aString.push_back(c);
								}
								break;
						}
					} else if (c == delimiter) {
						break;
					} else {
						aString.push_back(c);
					}
				}
			}
			auto eof = aStream.eof();
			aStream.clear();
			if (eof) {
				aStream.setstate(std::ios_base::eofbit);
			};
			return aStream;
		}
	} // end of namespace escapedIO

/// construct an object of type base

/// The newly created object is inserted into the maps sorted by long name and by short name,
/// (short name only if it is not '\0')
/// If a clash wopuld occur, i.e. either long or short name is already taken an exception is thrown.
/// \param [in] aShortName short option without the -, use '\0' to have only a long form
/// \param [in] aLongName long option without the --, must always be given
/// \param [in] aExplanation explanation for help output
/// \param [in] aNargs number of arguments/parameters. May be 0 ot 1.
	base::base(char aShortName, const std::string&  aLongName, const std::string&  aExplanation, short aNargs) :
		lShortName(aShortName),
		lLongName(aLongName),
		lExplanation(aExplanation),
		lNargs(aNargs) {

		{
			auto p = parser::fGetInstance();
			if (p != nullptr) {
				if (p->fIsParsingDone()) {
					std::string exceptionText(lLongName + " construction after parsing is done");
					throw std::logic_error(exceptionText);
				}
			}
		}


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

	/// remember the source that provided the value, e.g. commandline or a config file
	void base::fSetSource(const internal::sourceItem& aSource) {
		lSource = aSource;
	}


	void base::fHandleOption(int argc, const char *argv[], int *i) {
		if (*i + lNargs >= argc) {
			parser::fGetInstance()->fGetErrorStream() << "option " << lLongName << " needs " << lNargs << " args, but only " << argc - *i - 1 << " remain." << std::endl;
			parser::fGetInstance()->fComplainAndLeave();
		}
		if (lNargs == 0) {
			fSetMeNoarg(internal::sourceItem(&internal::sourceFile::gCmdLine, *i));
		} else if (lNargs == 1) {
			std::stringstream sbuf(argv[*i + 1]);
			fSetMe(sbuf, internal::sourceItem(&internal::sourceFile::gCmdLine, *i));
			*i += lNargs;
		}
		if (fCheckRange(parser::fGetInstance()->fGetErrorStream()) == false) {
			parser::fGetInstance()->fComplainAndLeave(false);
		}
	}

	void base::fWriteCfgLines(std::ostream & aStream, const char *aPrefix) const {
		aStream << aPrefix << lLongName << "=";
		auto asOriginalStringKeeper = dynamic_cast<const originalStringKeeper*>(this);
		if (asOriginalStringKeeper) {
			asOriginalStringKeeper->fWriteOriginalString(aStream);
		} else {
			fWriteValue(aStream);
		}
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

		if (!internal::positional_base::fGetPositonalArgs().empty()) {
			*lMessageStream << "Usage: " << lProgName << " [option]...";
			for (const auto& it : internal::positional_base::fGetPositonalArgs()) {
				*lMessageStream << " [" << it.second->fGetLongName() << "]";
				if (it.second->fIsContainer()) {
					*lMessageStream << "...";
				}
			}
			*lMessageStream << "\n";
		}

		size_t maxName = 0;
		size_t maxExplain = 0;
		size_t lineLenght = 132;
		#ifdef IS_NONBROKEN_SYSTEM
		{
			struct winsize window;
			if (ioctl(0, TIOCGWINSZ, &window) == 0) {
				lineLenght = window.ws_col;
			}
		}
		#endif
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

		*lMessageStream << lTrailer;
		*lMessageStream << std::endl;
	}
	void parser::fWriteCfgFile(const std::string& aFileName) {
		std::ofstream cfgFile(aFileName, std::ofstream::out | std::ofstream::trunc);
		if (lExecutableName.empty()) {
			#ifdef IS_NONBROKEN_SYSTEM
			char buf[128];
			auto result = readlink("/proc/self/exe", buf, sizeof(buf));
			if (result > 0 && result < 128 - 2 - 14) {
				cfgFile << "#!" << buf << " --readCfgFile\n";
			}
			#endif
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
			cfgFile << "# " << lProgName << " --noCfgFileRecursion --readCfgFile " << aFileName << " --writeCfgFile " << aFileName << "\n";
			cfgFile << "# Assuming " << lProgName << " is in your PATH\n";
		}
		for (auto & it : base::fGetOptionMap()) {
			const auto opt = it.second;
			if (opt->lPreserveWorthyStuff != nullptr) {
				for (const auto& line : * (opt->lPreserveWorthyStuff)) {
					cfgFile << "\n" << line;
				}
			}
			cfgFile << "\n# " << opt->lExplanation << "\n";
			auto prefix = "";
			if (opt->lSource.fIsUnset()) {
				prefix = "# ";
			}
			opt->fWriteCfgLines(cfgFile, prefix);
			opt->fWriteRange(cfgFile);
			if (!opt->lSource.fIsUnset()) {
				cfgFile << "# set from " << opt->lSource << "\n";
			}
		}
		cfgFile << "#\n";
		cfgFile.close();
	}

	void parser::fReadCfgFile(const std::string& aFileName, const internal::sourceItem& aSource, bool aMayBeAbsent) {
		std::ifstream cfgFile(aFileName);
		if (!cfgFile.good() && !aMayBeAbsent) {
			fGetErrorStream() << "can't acccess config file '" << aFileName << "', reason is " << strerror(errno) << std::endl;
			fComplainAndLeave(false);
		}
		auto sourceF = new internal::sourceFile(aFileName, *(aSource.fGetFile()));
		int lineNumber = 0;
		std::vector<std::string>* preserveWorthyStuff = nullptr;
		while (cfgFile.good()) {
			std::string line;
			std::getline(cfgFile, line);
			lineNumber++;
			internal::sourceItem source(sourceF, lineNumber);
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
				std::string buf;
				fReCaptureEscapedString(buf, line);
				lUnusedOptions.push_back(buf);
				continue;
			}
			auto optionName = line.substr(0, equalsAt);
			auto it = base::fGetOptionMap().find(optionName);
			if (it == base::fGetOptionMap().end()) {
				fGetErrorStream() << source << ": error: unknown option '" << optionName << "' ,line is '" << line << "'" << std::endl;
				continue;
			}
			auto option = it->second;
			{
				std::stringstream sbuf(line.substr(equalsAt + 1));
				option->fSetMe(sbuf, source);
				if (preserveWorthyStuff != nullptr) {
					option->fSetPreserveWorthyStuff(preserveWorthyStuff);
					preserveWorthyStuff = nullptr;
				}
				if (option->fCheckRange(fGetErrorStream()) == false) {
					fComplainAndLeave(false);
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
	void single<bool>::fSetMeNoarg(const internal::sourceItem& aSource) {
		lValue = ! lDefault;
		fSetSource(aSource);
	}
	void single<bool>::fSetMe(std::istream& aStream, const internal::sourceItem& aSource) {
		aStream >> std::boolalpha >> lValue;
		fSetSource(aSource);
	}
	void single<bool>::fAddDefaultFromStream(std::istream& aStream) {
		aStream >> std::boolalpha >> lValue;
		lDefault = lValue;
	}

	namespace internal { // put clases and options for internal use
		// into internal namespace

/// special derived class used to give help
		class OptionHelp : public single<bool> {
		  public:
			OptionHelp():
				single('h', "help", "give this help") {
			}
			void fSetMeNoarg(const sourceItem& /*aSource*/) override {
				parser::fGetInstance()->fHelp();
				exit(parser::fGetInstance()->fGetHelpReturnValue());
			}
			void fSetMe(std::istream& /*aArg*/, const sourceItem& aSource) override {
				fSetMeNoarg(aSource);
			}
		};
		static OptionHelp gHelp;


		/// special class for options which never have a value setting in cfg files
		template <typename T> class supressed : public single<T> {
		  public:
			template <class ... Types> supressed (Types ... args) :
				single<T>(args...) {
			};
			void fWriteCfgLines(std::ostream& aStream, const char */*aPrefix*/) const override {
				single<T>::fWriteCfgLines(aStream, "# ");
			}
		};

		/// standard option to suppress parsing of config files within config files
		class NoCfgFileRecursion: public supressed<bool> {
		  public:
			NoCfgFileRecursion():
				supressed('\0', "noCfgFileRecursion", "do not read config files recursively, must be set before use") {
			};
		};
		static NoCfgFileRecursion gNoCfgFileRecursion;


/// special derived class used to write out config files
		class OptionWriteCfgFile : public supressed<std::string> {
		  public:
			OptionWriteCfgFile():
				supressed('\0', "writeCfgFile", "write a config file", "") {
			}
			void fSetMe(std::istream& aStream, const sourceItem&/* aSource */) override {
				aStream >> *this;
				parser::fGetInstance()->fWriteCfgFile(*this);
				exit(parser::fGetInstance()->fGetHelpReturnValue());
			}
		};
		static OptionWriteCfgFile gWriteCfgFile;

/// special derived class used to read in config files
		class OptionReadCfgFile : public single<std::string> {
		  public:
			OptionReadCfgFile():
				single('\0', "readCfgFile", "read a config file", "") {
			}
			void fSetMe(std::istream& aStream, const sourceItem& aSource) override {
				single<std::string>::fSetMe(aStream, aSource);
				if (gNoCfgFileRecursion && aSource.fGetFile() != &sourceFile::gCmdLine) {
					return;
				}
				parser::fGetInstance()->fReadCfgFile(*this, aSource);
			};
			void fWriteCfgLines(std::ostream& aStream, const char */*aPrefix*/) const override {
				single<std::string>::fWriteCfgLines(aStream, gNoCfgFileRecursion ? "" : "# ");
			};
		};
		static OptionReadCfgFile gReadCfgFile;
	} // end of name internal

	void originalStringKeeper::fWriteOriginalString(std::ostream& aStream) const {
		aStream << lOriginalString;
	}

} // end of namespace options
