#ifndef __OptionsWithForeignParser_H__
#define __OptionsWithForeignParser_H__

#include "Options.h"

class ForeignOption;
class ForeignApplicationOptions {
  private:
	std::vector<const ForeignOption*> lOptions;
	std::string lName;
	std::vector<char *> lArgv;
	int lArgc;
	void fAddToArgc(const char *aString);
	void fAddToArgc(std::string& aString);
  public:
	ForeignApplicationOptions(const char* aName);
	void fRegister(const ForeignOption* aOption);
	void fFinalize();
	template <typename InputIt> void fFinalize(InputIt from, InputIt to) {
		fFinalize();
		for (auto it = from; it != to; ++it) {
			fAddToArgc(*it);
		}
		lArgc = lArgv.size();
	}
	char **Argv();
	int *Argc();
};

class ForeignOption : public OptionBase {
  private:
	std::string lCanonical;
	std::string lArg;
	ForeignApplicationOptions* lOptions;
  public:
	ForeignOption(ForeignApplicationOptions* aForeignApplication,
	              char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs,
	              const char *aCanonicalForm);
	virtual void fSetMe(const char *aArg, const char *aSource);
	virtual bool fCheckRange(std::ostream& aLogStream) const;
	virtual void fAddToRangeFromStream(std::istream& aStream);
	virtual void fAddDefaultFromStream(std::istream& aStream);
	virtual void fWriteValue(std::ostream& aStream) const;
	virtual ~ForeignOption();
	virtual void fAddArgs(std::vector<char *>& aArgv) const;
};

#endif
