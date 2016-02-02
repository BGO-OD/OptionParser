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
  private:
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
	virtual void fWriteDefault(std::ostream& aStream) const = 0;
  public:
	OptionBase(char aShortName, const std::string& aLongName, const std::string& aExplanation, short aNargs);
};

class OptionParser {
  protected:
	std::vector<std::string> lUnusedOptions;
	static OptionParser* gParser;
	const char *lDescription;
	const char *lProgName;
	void fReadConfigFiles();
  public:
	OptionParser(const char *aDescription = NULL);
	~OptionParser();
	const std::vector<std::string>& fParse(int argc, const char *argv[]);
	static OptionParser* fGetInstance();
	static void fHelp();
	static void fWriteCfgFile(const char *aFileName);
	static void fReadCfgFile(const char *aFileName, bool aMayBeAbsent = false);
	static void fPrintEscapedString(std::ostream &aStream, const char *aString);
	static void fReCaptureEscapedString(char *aDest, const char *aSource);
};

template <typename T> class Option : public OptionBase {
  private:
	T lValue;
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, T aDefault) :
		OptionBase(aShortName, aLongName, aExplanation, 1),
		lValue(aDefault) {
	}
	virtual void fWriteDefault(std::ostream& aStream) const {
		aStream << lValue;
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
	virtual void fWriteDefault(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);

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
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, const char* aDefault = NULL) :
		OptionBase(aShortName, aLongName, aExplanation, 1),
		lValue(aDefault) {
	}
	virtual void fWriteDefault(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);
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
  public:
	Option(char aShortName, const std::string& aLongName, const std::string& aExplanation, std::string aDefault = "") :
		OptionBase(aShortName, aLongName, aExplanation, 1),
		lValue(aDefault) {
	}
	virtual void fWriteDefault(std::ostream& aStream) const;
	virtual void fSetMe(const char *aArg);
	virtual void fSetFromStream(std::istream& aStream);
	operator const std::string& () const {
		return lValue;
	}
	const std::string& fGetValue() const {
		return lValue;
	}
};



#endif
