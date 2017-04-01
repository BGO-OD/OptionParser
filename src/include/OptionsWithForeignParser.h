#ifndef __OptionsWithForeignParser_H__
#define __OptionsWithForeignParser_H__

#include "Options.h"

namespace options {

class ForeignOption;
/// class to hold a list of options that are to be handled by a foreign option parser
class ForeignApplicationOptions {
  private:
	std::vector<const ForeignOption*> lOptions;
	std::string lName;
	std::vector<char *> lArgv;
	int lArgc;
  public:
	ForeignApplicationOptions(const char* aName);
	void fRegister(const ForeignOption* aOption);
	void fFinalize();
	/// \copydoc ForeignApplicationOptions::fFinalize() and adds strings from the iterators in the range [from,to)
	template <typename InputIt> void fFinalize(InputIt from, InputIt to) {
		fFinalize();
		for (auto it = from; it != to; ++it) {
			fAddToArgc(*it);
		}
	}
	void fAddToArgc(const char *aString);
	void fAddToArgc(const std::string& aString);
	char **Argv();
	int *Argc();
};

/// special type of option that is not really parsed but used to feed an additional option parser
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
	virtual void fAddArgs() const;
};

} // end of namespace options

#endif
