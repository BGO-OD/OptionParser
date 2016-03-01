#ifndef __Options_H__
#define __Options_H__



#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string.h>


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
	virtual void fSetMe(const char *aArg) = 0;
  private:
	virtual void fHandleOption(int argc, const char *argv[], int *i);

  protected:
	virtual void fWriteCfgLines(std::ostream& aStream) const;
	virtual bool fCheckRange(std::ostream& aLogStream) const = 0;
	virtual void fWriteRange(std::ostream &/*aStream*/) const {};
	void fSetPreserveWorthyStuff(std::vector<std::string>* aStuff);

  public:
	OptionBase(char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs);
	virtual void fAddToRangeFromStream(std::istream& aStream) = 0;
	virtual void fWriteValue(std::ostream& aStream) const = 0;
	const std::string& fGetLongName() const {
		return lLongName;
	};
};

class OptionParser {
  protected:
	static OptionParser* gParser;
	const char *lDescription;
	const char *lTrailer;
	const std::vector<std::string> lSearchPaths;
	std::vector<std::string> lUnusedOptions;
	std::vector<std::string> lStuffAfterMinusMinus;
	bool lMinusMinusJustEndsOptions;
	std::ostream *lMessageStream;
	std::string lProgName;
	std::string lExecutableName;
	int lHelpReturnValue;
	char lPrimaryAssignment;
	char lSecondaryAssignment;
	void fReadConfigFiles();
  public:
	OptionParser(const char *aDescription = NULL, const char *aTrailer = NULL, const std::vector<std::string> aSearchPaths = {"/etc/", "~/.", "~/.config/", "./."});
	~OptionParser();
	void fSetMessageStream(std::ostream* aStream);
	void fSetHelpReturnValue(int aValue);
	int fGetHelpReturnValue() const {
		return lHelpReturnValue;
	};
	void fSetAssignmentChars(char aPrimary = '=', char aSecondary = ':');
	char fGetSecondaryAssignment() const {
		return lSecondaryAssignment;
	}
	const std::vector<std::string>& fParse(int argc, const char *argv[]);
	const std::vector<std::string>& fParse(int argc, char *argv[]);
	static OptionParser* fGetInstance();
	void fHelp();
	void fWriteCfgFile(const char *aFileName);
	void fReadCfgFile(const char *aFileName, bool aMayBeAbsent = false);
	void fSetMinusMinusStartsExtraList();
	void fSetExecutableName(const char *aName);
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


	virtual void fSetMe(const char* aArg) {
		std::stringstream buf(aArg);
		buf >> std::noskipws >> lValue;
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
	virtual void fSetRange(const std::vector<const char *>& aRange);
	virtual void fAddToRangeFromStream(std::istream& aStream);
	virtual void  fWriteRange(std::ostream &aStream) const;
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);
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
	virtual void  fWriteRange(std::ostream &aStream) const;
	virtual bool fCheckRange(std::ostream& aLogStream) const;
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);
	operator const std::string& () const {
		return lValue;
	}
	const std::string& fGetValue() const {
		return lValue;
	}
};

template <typename T> class OptionMap: public OptionBase, public std::map<std::string, T> {
  public:
	OptionMap(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBase(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};

	virtual void fWriteCfgLines(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << lLongName << "=key" << OptionParser::fGetInstance()->fGetSecondaryAssignment() << "value\n";
		}
		for (auto it = this->begin(); it != this->end(); ++it) {
			aStream << lLongName << "=" << it->first <<  OptionParser::fGetInstance()->fGetSecondaryAssignment() << it->second << "\n";
		}
	}

	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}

	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "no value";
		} else {
			for (auto it = this->begin(); it != this->end(); ++it) {
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
		(*this)[name] = value;
	}
	const std::map<std::string, T>& fGetValue() const  {
		return *static_cast<const std::map<std::string, T>*>(this);
	}
};
template <> class OptionMap<std::string>: public OptionBase, public std::map<std::string, std::string> {
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

	const  std::map<std::string, std::string>& fGetValue() const {
		return *static_cast<const std::map<std::string, std::string>*> (this);
	}
};


template <typename T, typename Container = std::vector<T>> class OptionContainer: public OptionBase, public Container {
  public:
	OptionContainer(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBase(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fWriteCfgLines(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << lLongName << "=value\n";
		}
		for (auto it = this->begin(); it != this->end(); ++it) {
			aStream << lLongName << "=" << *it << "\n";
		}
	}
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}


	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "no value";
		} else {
			for (auto it = this->begin(); it != this->end(); ++it) {
				aStream << *it << " ";
			}
		}
	}
	virtual void fSetMe(const char *aArg) {
		std::stringstream valueStream(aArg);
		T value;
		valueStream >> value;
		this->push_back(value);
	}

	operator const Container& () const {
		return *static_cast<const Container*>(this);
	}
	const Container& fGetValue() const  {
		return *static_cast<const Container*>(this);
	}

};






template <typename Container> class OptionContainer<const char *, Container>: public OptionBase, public Container {
  public:
	OptionContainer(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBase(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fWriteCfgLines(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << lLongName << "=value\n";
		}
		for (auto it = this->begin(); it != this->end(); ++it) {
			aStream << lLongName << "=";
			OptionParser::fPrintEscapedString(aStream, *it);
			aStream << "\n";
		}
	}
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}


	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "no value";
		} else {
			for (auto it = this->begin(); it != this->end(); ++it) {
				OptionParser::fPrintEscapedString(aStream, *it);
			}
		}
	}
	virtual void fSetMe(const char *aArg) {
		auto buf = new char[strlen(aArg) + 1];
		OptionParser::fReCaptureEscapedString(buf, aArg);
		this->push_back(buf);
	}
	operator const Container & () const {
		return *static_cast<const Container*>(this);
	}
	const Container& fGetValue() const  {
		return *static_cast<const Container*>(this);
	}

};


template <typename Container> class OptionContainer<std::string, Container>: public OptionBase, public Container {
  public:
	OptionContainer(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
		OptionBase(aShortName, aLongName, aExplanation, 1) {
	}
	virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
	virtual void fWriteCfgLines(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << lLongName << "=value\n";
		}
		for (auto it = this->begin(); it != this->end(); ++it) {
			aStream << lLongName << "=";
			OptionParser::fPrintEscapedString(aStream, it->c_str());
			aStream << "\n";
		}
	}
	virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}


	virtual void fWriteValue(std::ostream& aStream) const {
		if (this->empty()) {
			aStream << "no value";
		} else {
			for (auto it = this->begin(); it != this->end(); ++it) {
				OptionParser::fPrintEscapedString(aStream, it->c_str());
			}
		}
	}
	virtual void fSetMe(const char* aArg) {
		auto buf = new char[strlen(aArg) + 1];
		OptionParser::fReCaptureEscapedString(buf, aArg);
		this->push_back(buf);
	}
	operator const Container & () const {
		return *static_cast<const Container*>(this);
	}
	const Container& fGetValue() const  {
		return *static_cast<const Container*>(this);
	}

};


#endif
