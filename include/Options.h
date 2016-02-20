#ifndef __Options_H__
#define __Options_H__



#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#define debug() std::cerr << __FILE__ << ":" << __LINE__ << ":" << std::endl

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
  protected:
	short lNargs;
	virtual void fSetMe(const char *aArg);
	virtual void fSetFromStream(std::istream& aStream) = 0;
  private:
	virtual void fHandleOption(int argc, const char *argv[], int *i);

  protected:
	virtual void fWriteCfgLines(std::ostream& aStream) const;
	virtual bool fCheckRange(std::ostream& aLogStream) const = 0;
	virtual void fWriteRange(std::ostream &/*aStream*/) const {};

  public:
	OptionBase(char aShortName, const std::string& aLongName, const std::string& aExplanation, short aNargs);
	virtual void fAddToRangeFromStream(std::istream& aStream) = 0;
	virtual void fWriteValue(std::ostream& aStream) const = 0;
	const std::string& fGetLongName() const {
		return lLongName;
	};
};

class OptionParser {
  protected:
	std::vector<std::string> lUnusedOptions;
	static OptionParser* gParser;
	const char *lDescription;
	const char *lProgName;
	std::ostream *lMessageStream;
	int lHelpReturnValue;
	char lPrimaryAssignment;
	char lSecondaryAssignment;
	void fReadConfigFiles();
  public:
	OptionParser(const char *aDescription = NULL);
	~OptionParser();
	void fSetMessageStream(std::ostream* aMessageStream);
	void fSetHelpReturnValue(int aValue);
	int fGetHelpReturnValue() const {
		return lHelpReturnValue;
	};
	void fSetAssignmentChars(char aPrimary = '=', char aSecondary = ':');
	char fGetSecondaryAssignment() const {
		return lSecondaryAssignment;
	}
	const std::vector<std::string>& fParse(int argc, const char *argv[]);
	static OptionParser* fGetInstance();
	void fHelp();
	void fWriteCfgFile(const char *aFileName);
	void fReadCfgFile(const char *aFileName, bool aMayBeAbsent = false);
	static void fPrintEscapedString(std::ostream &aStream, const char *aString);
	static void fReCaptureEscapedString(char *aDest, const char *aSource);
};

template <typename T> class Option : public OptionBase {
  private:
	T lValue;
	std::vector<T> lRange;
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, T aDefault, const std::vector<T>* aRange = NULL) :
		OptionBase(aShortName, aLongName, aExplanation, 1),
		lValue(aDefault) {
		if (aRange != NULL) {
			fSetRange(*aRange);
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


	virtual void fSetFromStream(std::istream& aStream) {
		aStream >> std::noskipws >> lValue;
	}
	operator T () const {
		return lValue;
	}
	T &fGetValue() const {
		return lValue;
	}
};
template <> class Option<bool> : public OptionBase {
  private:
	bool lValue;
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, bool aDefault = false) :
		OptionBase(aShortName, aLongName, aExplanation, 0),
		lValue(aDefault) {
	}
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	};
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};

	virtual void fSetFromStream(std::istream& aStream);
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
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, const char* aDefault = NULL, const std::vector<const char *>* aRange = NULL);
	virtual void fSetRange(const std::vector<const char *>& aRange);
	virtual void fAddToRangeFromStream(std::istream& aStream);
	virtual void  fWriteRange(std::ostream &aStream) const;
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);
	virtual bool fCheckRange(std::ostream& aLogStream) const;
	virtual void fSetFromStream(std::istream& aStream);
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
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, const std::string& aDefault = "", const std::vector<std::string>* aRange = NULL);
	virtual void fSetRange(const std::vector<std::string>& aRange);
	virtual void fAddToRangeFromStream(std::istream& aStream);
	virtual void  fWriteRange(std::ostream &aStream) const;
	virtual bool fCheckRange(std::ostream& aLogStream) const;
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);
	virtual void fSetFromStream(std::istream& aStream);
	operator const std::string& () const {
		return lValue;
	}
	const std::string& fGetValue() const {
		return lValue;
	}
};

template <typename T> class OptionMap: public OptionBase {
  private:
	std::map<std::string, T> lValueMap;
  public:
	OptionMap(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBase(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};

	virtual void fWriteCfgLines(std::ostream& aStream) const {
		if (lValueMap.empty()) {
			aStream << lLongName << "=key" << OptionParser::fGetInstance()->fGetSecondaryAssignment() << "value\n";
		}
		for (auto it = lValueMap.begin(); it != lValueMap.end(); ++it) {
			aStream << lLongName << "=" << it->first <<  OptionParser::fGetInstance()->fGetSecondaryAssignment() << it->second << "\n";
		}
	}

	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}

	virtual void fWriteValue(std::ostream& aStream) const {
		if (lValueMap.empty()) {
			aStream << "no value";
		} else {
			for (auto it = lValueMap.begin(); it != lValueMap.end(); ++it) {
				aStream << it->first << ":" << it->second << " ";
			}
		}
	}
	virtual void fSetMe(const char *aArg) {
		std::string s(aArg);
		auto dividerPosition = s.find_first_of(':');
		auto name = s.substr(0, dividerPosition);
		std::stringstream valueStream(s.substr(dividerPosition + 1, std::string::npos));
		T value;
		valueStream >> value;
		lValueMap[name] = value;
	}
	virtual void fSetFromStream(std::istream& /*aStream*/) {
	}
	operator const decltype(lValueMap)& () const {
		return lValueMap;
	}
	auto fGetValue() const -> const decltype(lValueMap)& {
		return lValueMap;
	}
	auto begin() const -> decltype(lValueMap.cbegin()) const {
		return lValueMap.cbegin();
	}
	auto end() const -> decltype(lValueMap.cend()) const {
		return lValueMap.cend();
	}
};
template <> class OptionMap<std::string>: public OptionBase {
  private:
	std::map<std::string, std::string> lValueMap;
  public:
	OptionMap(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBase(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fWriteCfgLines(std::ostream& aStream) const;
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	};

	virtual void fSetFromStream(std::istream& /*aStream*/) {
	}
	operator const decltype(lValueMap)& () const {
		return lValueMap;
	}
	auto fGetValue() const -> const decltype(lValueMap)& {
		return lValueMap;
	}
	auto begin() const -> decltype(lValueMap.cbegin()) const {
		return lValueMap.cbegin();
	}
	auto end() const -> decltype(lValueMap.cend()) const {
		return lValueMap.cend();
	}
};


#endif
