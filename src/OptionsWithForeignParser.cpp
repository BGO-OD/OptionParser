#include "OptionsWithForeignParser.h"
#include <limits>
namespace options {
/// construct an object of typs ForeignApplicationOptions

/// in a derived class the constructor should generete via new the
/// ForeignOption objects that are recognized by the corresponing parser.
	ForeignApplicationOptions::ForeignApplicationOptions(const char* aName) :
		lName(aName) {
		lArgc = 0;
	}

/// add aOption to the list of options that were set.
	void ForeignApplicationOptions::fRegister(const ForeignOption* aOption) {
		lOptions.push_back(aOption);
	}

/// build the internal argument vector for the call to the external parser

/// This function must be called before Argc() and Argv() may be called.
/// it adds all the canonical forms of the options thate were set to the argument vector.
	void ForeignApplicationOptions::fFinalize() {
		lArgv.clear();
		lArgv.push_back(const_cast<char*>(lName.data()));
		for (auto opt : lOptions) {
			opt->fAddArgs();
		}
	}
/// add string to the argument vector for the external parser
	void ForeignApplicationOptions::fAddToArgc(const char *aString) {
		lArgv.push_back(const_cast<char*>(aString));
		lArgc = lArgv.size();
	}
/// add string to the argument vector for the external parser
	void ForeignApplicationOptions::fAddToArgc(const std::string& aString) {
		lArgv.push_back(const_cast<char*>(aString.data()));
		lArgc = lArgv.size();
	}
/// return an argument vector for use with the external parser
	char ** ForeignApplicationOptions::Argv() {
		return lArgv.data();
	}
/// return a pointer to the argument count for use with the external parser
	int * ForeignApplicationOptions::Argc() {
		return &lArgc;
	}

/// \copydetails base::base()
/// \param [in] aForeignApplication required pointer to the ForeignApplicationOptions object that this option shall belong to
/// \param [in] aCanonicalForm canonical form of the option, including leading - signs to be given to the other parser
	ForeignOption::ForeignOption(ForeignApplicationOptions* aForeignApplication,
	                             char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs,
	                             const char *aCanonicalForm) :
		base(aShortName, aLongName, aExplanation, aNargs),
		lCanonical(aCanonicalForm), lOptions(aForeignApplication) {
	}
	ForeignOption::~ForeignOption() {}
/// remember argument ad register in list of options that were set.
	void ForeignOption::fSetMe(const char *aArg, const internal::sourceItem& aSource) {
		if (aArg != nullptr) {
			lArg = aArg;
		}
		fSetSource(aSource);
		lOptions->fRegister(this);
	}
/// add args of this option tho the argument vector of the option list
	void ForeignOption::fAddArgs() const {
		lOptions->fAddToArgc(lCanonical);
		if (lNargs == 1) {
			lOptions->fAddToArgc(lArg);
		}
	}
/// range checking does not work, we know no ranges
	bool ForeignOption::fCheckRange(std::ostream& /*aLogStream*/) const {
		return true;
	}
/// just read away the line, we cannot do anything sensible here
	void ForeignOption::fAddToRangeFromStream(std::istream& aStream) {
		aStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
/// just read away the line, we cannot do anything sensible here
	void ForeignOption::fAddDefaultFromStream(std::istream& aStream) {
		aStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	void ForeignOption::fWriteValue(std::ostream& aStream) const {
		aStream << lArg;
	}
} // end of namespace options
