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
#ifndef __Options_H__
#define __Options_H__

#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>


/// base class for options

/// Only the templated classes that derive from this base class can contain values.
/// This base class only contains members that are generally usable and do not depend
/// on the type of the option.
class OptionBase {
	friend class OptionParser;
  protected:
	static std::map<std::string, OptionBase*>& fGetOptionMap() {
		static std::map<std::string, OptionBase*> gOptions;
		return gOptions;
	};
	static std::map<char, OptionBase*>& fGetShortOptionMap() {
		static std::map<char, OptionBase*> gShortOptions;
		return gShortOptions;
	};
  protected:
	char lShortName;
	const std::string lLongName;
	const std::string lExplanation;
	std::string lSource;
	short lNargs;
	std::vector<std::string>* lPreserveWorthyStuff;

	std::vector<const OptionBase*> lRequiredOptions;
	std::vector<const OptionBase*> lForbiddenOptions;
	virtual void fSetMe(const char *aArg, const char *aSource) = 0;
	virtual void fSetSource(const char *aSource);
  private:
	virtual void fHandleOption(int argc, const char *argv[], int *i);

  protected:
	virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const;
	virtual bool fCheckRange(std::ostream& aLogStream) const = 0;
	virtual void fWriteRange(std::ostream &/*aStream*/) const {};
	void fSetPreserveWorthyStuff(std::vector<std::string>* aStuff);

  public:
	OptionBase(char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs);
	virtual ~OptionBase();

	/// special for use in the shellScriptOptionParser
	virtual void fAddToRangeFromStream(std::istream& aStream) = 0; ///< read values from aStream and add them to the range vector
	/// special for use in the shellScriptOptionParser
	virtual void fAddDefaultFromStream(std::istream& aStream) = 0; ///< read a value from aStram and set that as default
	virtual void fWriteValue(std::ostream& aStream) const = 0;
	virtual void fRequire(const OptionBase* aOtherOption);
	virtual void fRequire(std::vector<const OptionBase*> aOtherOptions);
	virtual void fForbid(const OptionBase* aOtherOption);
	virtual void fForbid(std::vector<const OptionBase*> aOtherOptions);
	virtual bool fIsSet() const {
		return ! lSource.empty();
	};
	const std::string& fGetLongName() const {
		return lLongName;
	};
};



/// class that contains the parser, i.e. does that option handling

/// once contructed with the header part and the trailer part of the help text,
/// together with the list of search paths for config files the option parser
/// can then be used to parse the command line options.

class OptionParser {
  protected:
	static OptionParser* gParser;
	const char *lDescription;
	const char *lTrailer;
	const std::vector<std::string> lSearchPaths;
	std::vector<std::string> lUnusedOptions;
	std::vector<std::string> lStuffAfterMinusMinus;

	std::set<const OptionBase*> lRequiredOptions;

	bool lMinusMinusJustEndsOptions;
	std::ostream *lMessageStream;
	std::ostream *lErrorStream;
	std::string lProgName;
	std::string lExecutableName;
	int lHelpReturnValue;
	char lPrimaryAssignment;
	char lSecondaryAssignment;

	void fReadConfigFiles();
	void fPrintOptionHelp(std::ostream& aMessageStream, const OptionBase& aOption, std::size_t aMaxName, std::size_t aMaxExplain, size_t lineLenght) const;
	void fCheckConsistency();
  public:
	OptionParser(const char *aDescription = NULL, const char *aTrailer = NULL, const std::vector<std::string>& aSearchPaths = {"/etc/", "~/.", "~/.config/", "./."});
	~OptionParser();
	void fSetMessageStream(std::ostream* aStream);
	void fSetErrorStream(std::ostream* aStream);
	std::ostream& fGetErrorStream() const;
	void fSetHelpReturnValue(int aValue);
	int fGetHelpReturnValue() const {
		return lHelpReturnValue;
	};
	[[noreturn]] virtual void fComplainAndLeave(bool aWithHelp = true);
	void fSetAssignmentChars(char aPrimary = '=', char aSecondary = ':');
	char fGetSecondaryAssignment() const {
		return lSecondaryAssignment;
	}
	virtual void fRequire(const OptionBase* aOtherOption);
	virtual void fRequire(std::vector<const OptionBase*> aOtherOptions);


	/// parse the options on the command line

	/// The command line is passed via the argc and argv parameres, which will reamain unchanged.
	/// Also reads config files, using fReadConfigFiles() and all files which are explicitly requested by  --readCfgFile options.
	/// \result vector of strings containing all the unhandled string from the original command line
	const std::vector<std::string>& fParse(int argc, const char *argv[]);

	/// other signature for fParse, when argv needs to be without const due to external constraints
	const std::vector<std::string>& fParse(int argc, char *argv[]);


	/// get the only allwed instance of the option parser.
	static OptionParser* fGetInstance();

	/// print help, normally automatically called by the --help option or in case of problems.
	void fHelp();
	void fWriteCfgFile(const char *aFileName);
	void fReadCfgFile(const char *aFileName, bool aMayBeAbsent = false);
	void fSetExecutableName(const char *aName);

	/// switch on use of -- to separate a trailer on the command line that is not to be parsed
	void fSetMinusMinusStartsExtraList();
	/// get trailong part of command line as a vector od strings, requires that fSetMinusMinusStartsExtraList was called before parsing.
	const std::vector<std::string>& fGetStuffAfterMinusMinus() {
		return lStuffAfterMinusMinus;
	};
	static void fPrintEscapedString(std::ostream &aStream, const char *aString);
	static void fReCaptureEscapedString(char *aDest, const char *aSource);
};



template <typename T> class Option : public OptionBase {
  private:
	T lValue;
	std::vector<T> lRange;
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, T aDefault, const std::vector<T>& aRange = {}) :
		OptionBase(aShortName, aLongName, aExplanation, 1),
		lValue(aDefault) {
		if (!aRange.empty()) {
			fSetRange(aRange);
		}
	}
	virtual void fSetRange(const std::vector<T>& aRange) {
		for (auto it = aRange.begin(); it != aRange.end(); ++it) {
			lRange.push_back(*it);
		}
	}
	virtual void fAddToRangeFromStream(std::istream& aStream) {
		std::string buf;
		std::getline(aStream, buf);
		std::stringstream sbuf(buf);
		while (!sbuf.eof()) {
			T value;
			sbuf >> value;
			lRange.push_back(value);
		}
	}
	virtual void fAddDefaultFromStream(std::istream& aStream) {
		aStream >> lValue;
	}
	virtual void  fWriteRange(std::ostream &aStream) const {
		if (! lRange.empty()) {
			aStream << "# allowed range is";
			if (lRange.size() == 2) {
				aStream << " [" << lRange[0] << "," << lRange[1] << "]\n";
			} else {
				aStream << ":";
				for (auto it = lRange.begin(); it != lRange.end(); ++it) {
					aStream << " " << *it;
				}
				aStream << "\n";
			}
		}
	}

	virtual void fWriteValue(std::ostream& aStream) const {
		aStream << lValue;
	}
	virtual bool fCheckRange(std::ostream& aLogStream) const {
		if (lRange.empty()) {
			return true;
		} else if (lRange.size() == 2) {
			if (lRange[0] <= lValue && lValue <= lRange[1]) {
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


	virtual void fSetMe(const char* aArg, const char* aSource) {
		std::stringstream buf(aArg);
		buf >> std::noskipws >> lValue;
		fSetSource(aSource);
	}
	operator T () const {
		return lValue;
	}
	const T &fGetValue() const {
		return lValue;
	}
};
template <> class Option<bool> : public OptionBase {
  private:
	bool lValue;
	bool lDefault;
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, bool aDefault = false) :
		OptionBase(aShortName, aLongName, aExplanation, 0),
		lValue(aDefault), lDefault(aDefault) {
	}
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg, const char* aSource);
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	};
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fAddDefaultFromStream(std::istream& aStream);

	operator bool () const {
		return lValue;
	}
	bool fGetValue() const {
		return lValue;
	}
};
template <> class Option<const char *> : public OptionBase {
  protected:
	const char *lValue;
	std::vector<const char*> lRange;
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, const char* aDefault = NULL, const std::vector<const char *>& aRange = {});
	virtual ~Option();
	virtual void fSetRange(const std::vector<const char *>& aRange);
	virtual void fAddToRangeFromStream(std::istream& aStream);
	virtual void fAddDefaultFromStream(std::istream& aStream);
	virtual void  fWriteRange(std::ostream &aStream) const;
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg, const char* aSource);
	virtual bool fCheckRange(std::ostream& aLogStream) const;
	operator const char* () const {
		return lValue;
	}
	const char *fGetValue() const {
		return lValue;
	}
};
template <> class Option<std::string> : public OptionBase {
  protected:
	std::string lValue;
	std::vector<std::string> lRange;
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, std::string  aDefault = "", const std::vector<std::string>& aRange = {});
	virtual void fSetRange(const std::vector<std::string>& aRange);
	virtual void fAddToRangeFromStream(std::istream& aStream);
	virtual void fAddDefaultFromStream(std::istream& aStream);
	virtual void  fWriteRange(std::ostream &aStream) const;
	virtual bool fCheckRange(std::ostream& aLogStream) const;
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg, const char* aSource);
	operator const std::string& () const {
		return lValue;
	}
	const std::string& fGetValue() const {
		return lValue;
	}
};



/// This class is an intermediate helper class for options that
/// are map-based. It is not to be used directly.
template <typename T> class OptionBaseForMap: public OptionBase {
  protected:
	std::map<const T*, std::string> lSources;
  public:
	OptionBaseForMap(char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs) :
		OptionBase(aShortName, aLongName, aExplanation, aNargs) {};
	void fAddSource(const T* aValueLocation, std::string aSource) {
		lSources.insert(std::make_pair(aValueLocation, aSource));
	};
	const char *fGetSource(const T* aValueLocation) const {
		auto it = lSources.find(aValueLocation);
		if (it != lSources.end()) {
			return it->second.c_str();
		} else {
			return nullptr;
		}
	};
	virtual bool fIsSet() const {
		return ! lSources.empty();
	};
};

/// template for map-based options. The map key is always a std::string but the mapped value is arbitrary.
/// the container is by defalt a std::map. It is assumed that the container always containds std::pairs
/// of a std::string as first and the valye type T as second, e.g. a
/// std::list<std::pair<std::string,int>> which, in contrast to the map would preserve the order in
/// which the items were specified.
template <typename T, typename Container = std::map<std::string, T>> class OptionMap: public OptionBaseForMap<T>, public Container {
  public:
	OptionMap(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBaseForMap<T>(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fAddDefaultFromStream(std::istream& /*aStream*/) {};

	virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const {
		if (this->empty()) {
			aStream << aPrefix << this->lLongName << "=key" << OptionParser::fGetInstance()->fGetSecondaryAssignment() << "value\n";
		}
		for (const auto& it : *this) {
			auto source = this->fGetSource(&(it.second));
			aStream << (source ? "" : aPrefix) << this->lLongName << "=" << it.first <<  OptionParser::fGetInstance()->fGetSecondaryAssignment() << it.second << "\n";
			if (source) {
				aStream << "# set from " << source << "\n";
			}
		}
	}

	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}

	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "\"\"";
		} else {
			for (const auto& it : *this) {
				aStream << it.first << ":" << it.second << " ";
			}
		}
	}
	virtual void fSetMe(const char *aArg, const char* aSource) {
		std::string s(aArg);
		auto dividerPosition = s.find_first_of(':');
		auto name = s.substr(0, dividerPosition);
		std::stringstream valueStream(s.substr(dividerPosition + 1, std::string::npos));
		T value;
		valueStream >> value;
		auto result = (*this).insert(this->end(), std::make_pair(name, value));
		this->fAddSource(&(result->second), aSource);
	}
	const std::map<std::string, T>& fGetValue() const  {
		return *static_cast<const std::map<std::string, T>*>(this);
	}
};




/// template specialisation for maps where the values are also std::strings
template <typename Container> class OptionMap<std::string, Container>: public OptionBaseForMap<std::string>, public Container {
  public:
	OptionMap(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBaseForMap<std::string>(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fAddDefaultFromStream(std::istream& /*aStream*/) {};

	virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const {
		if (this->empty()) {
			aStream << aPrefix << this->lLongName << "=key" << OptionParser::fGetInstance()->fGetSecondaryAssignment() << "value\n";
		}
		for (const auto & it : *this) {
			auto source = this->fGetSource(&(it.second));

			aStream << (source ? "" : aPrefix) << this->lLongName << "=" << it.first << OptionParser::fGetInstance()->fGetSecondaryAssignment();
			OptionParser::fPrintEscapedString(aStream, it.second.c_str());
			aStream << "\n";
			if (source) {
				aStream << "# set from " << source << "\n";
			}
		}
	};
	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "\"\"";
		} else {
			for (auto it = this->begin(); it != this->end(); ++it) {
				if (it != this->begin()) {
					aStream << ',';
				}
				aStream << it->first << ":" << it->second << " ";
			}
		}
	};

	virtual void fSetMe(const char *aArg, const char *aSource) {
		std::string s(aArg);
		auto dividerPosition = s.find_first_of(OptionParser::fGetInstance()->fGetSecondaryAssignment());
		auto name = s.substr(0, dividerPosition);
		auto buf = new char[s.length() - dividerPosition];
		OptionParser::fReCaptureEscapedString(buf, s.substr(dividerPosition + 1, std::string::npos).c_str());
		auto result = (*this).insert(this->end(), std::make_pair(name, buf));
		fAddSource(&(result->second), aSource);
	};
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	};

	const  Container& fGetValue() const {
		return *static_cast<Container*> (this);
	}
};

/// This class is an intermediate helper class for options that
/// are container-based. It is not to be used directly.
class OptionBaseForContainer: public OptionBase {
  protected:
	std::vector<std::string> lSources;
  public:
	OptionBaseForContainer(char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs) :
		OptionBase(aShortName, aLongName, aExplanation, aNargs) {};
	virtual bool fIsSet() const {
		return ! lSources.empty();
	};
};

/// template for container-based options.
/// the container is by defalt a std::vector.
/// If a non-vector container is used it needs to have a push_back.
template <typename T, typename Container = std::vector<T>> class OptionContainer: public OptionBaseForContainer, public Container {
  public:
	OptionContainer(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBaseForContainer(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fAddDefaultFromStream(std::istream& /*aStream*/) {};

	virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const {
		if (this->empty()) {
			aStream << aPrefix << this->lLongName << "=value\n";
		}
		auto it2 = lSources.begin();
		for (auto it = this->begin(); it != this->end(); ++it, ++it2) {
			aStream << (it2->empty() ? aPrefix : "") << this->lLongName << "=" << *it << "\n";
			if (!it2->empty()) {
				aStream << "# set from " << *it2 << "\n";
			}
		}
	}
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}


	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "\"\"";
		} else {
			for (auto it = this->begin(); it != this->end(); ++it) {
				if (it != this->begin()) {
					aStream << ',';
				}
				aStream << *it << " ";
			}
		}
	}
	virtual void fSetMe(const char *aArg, const char *aSource) {
		std::stringstream valueStream(aArg);
		T value;
		valueStream >> value;
		this->push_back(value);
		lSources.push_back(aSource ? aSource : "");
	}

	operator const Container& () const {
		return *static_cast<const Container*>(this);
	}
	const Container& fGetValue() const  {
		return *static_cast<const Container*>(this);
	}

};





/// template specialisation for container based options that contain const char* strings
template <typename Container> class OptionContainer<const char *, Container>: public OptionBaseForContainer, public Container {
  public:
	OptionContainer(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBaseForContainer(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fAddDefaultFromStream(std::istream& /*aStream*/) {};

	virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const {
		if (this->empty()) {
			aStream << aPrefix << this->lLongName << "=value\n";
		}
		auto it2 = lSources.begin();
		for (auto it = this->begin(); it != this->end(); ++it, ++it2) {
			aStream << (it2->empty() ? aPrefix : "") << this->lLongName << "=";
			OptionParser::fPrintEscapedString(aStream, *it);
			aStream << "\n";
			if (!it2->empty()) {
				aStream << "# set from " << *it2 << "\n";
			}
		}
	}
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}


	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "\"\"";
		} else {
			for (auto it = this->begin(); it != this->end(); ++it) {
				if (it != this->begin()) {
					aStream << ',';
				}
				OptionParser::fPrintEscapedString(aStream, *it);
			}
		}
	}
	virtual void fSetMe(const char *aArg, const char *aSource) {
		auto buf = new char[strlen(aArg) + 1];
		OptionParser::fReCaptureEscapedString(buf, aArg);
		this->push_back(buf);
		lSources.push_back(aSource ? aSource : "");
	}
	operator const Container & () const {
		return *static_cast<const Container*>(this);
	}
	const Container& fGetValue() const  {
		return *static_cast<const Container*>(this);
	}

};

/// template specialisation for container based options that contain std::strings
template <typename Container> class OptionContainer<std::string, Container>: public OptionBaseForContainer, public Container {
  public:
	OptionContainer(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBaseForContainer(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& aStream) {
		std::string buf1;
		std::getline(aStream, buf1);
		auto buf2 = new char[buf1.length() + 1];
		OptionParser::fReCaptureEscapedString(buf2, buf1.c_str());
		//	lRange.push_back(buf2);
	};
	virtual void fAddDefaultFromStream(std::istream& aStream) {
		std::string buf1;
		std::getline(aStream, buf1);
		fSetMe(buf1.c_str(), nullptr);
	};

	virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const {
		if (this->empty()) {
			aStream << aPrefix << this->lLongName << "=value\n";
		}
		auto it2 = lSources.begin();
		for (auto it = this->begin(); it != this->end(); ++it, ++it2) {
			aStream << (it2->empty() ? aPrefix : "") << this->lLongName << "=";
			OptionParser::fPrintEscapedString(aStream, it->c_str());
			aStream << "\n";
			if (!it2->empty()) {
				aStream << "# set from " << *it2 << "\n";
			}
		}
	}
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}


	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "\"\"";
		} else {
			for (auto it = this->begin(); it != this->end(); ++it) {
				if (it != this->begin()) {
					aStream << ',';
				}
				OptionParser::fPrintEscapedString(aStream, it->c_str());
			}
		}
	}
	virtual void fSetMe(const char* aArg, const char *aSource) {
		auto buf = new char[strlen(aArg) + 1];
		OptionParser::fReCaptureEscapedString(buf, aArg);
		this->push_back(buf);
		lSources.push_back(aSource ? aSource : "");
	}
	operator const Container & () const {
		return *static_cast<const Container*>(this);
	}
	const Container& fGetValue() const  {
		return *static_cast<const Container*>(this);
	}

};


#endif
