#include "OptionsWithForeignParser.h"
ForeignApplicationOptions::ForeignApplicationOptions(const char* aName) :
	lName(aName) {
}
void ForeignApplicationOptions::fRegister(const ForeignOption* aOption) {
	lOptions.push_back(aOption);
}
void ForeignApplicationOptions::fFinalize() {
	lArgv.clear();
	lArgv.push_back(const_cast<char*>(lName.data()));
	for (auto opt : lOptions) {
		opt->fAddArgs(lArgv);
	}
	lArgc = lArgv.size();
}
void ForeignApplicationOptions::fAddToArgc(const char *aString) {
	lArgv.push_back(const_cast<char*>(aString));
}
void ForeignApplicationOptions::fAddToArgc(std::string& aString) {
	lArgv.push_back(const_cast<char*>(aString.data()));
}

char ** ForeignApplicationOptions::Argv() {
	return lArgv.data();
}
int * ForeignApplicationOptions::Argc() {
	return &lArgc;
}


ForeignOption::ForeignOption(ForeignApplicationOptions* aForeignApplication,
                             char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs,
                             const char *aCanonicalForm) :
	OptionBase(aShortName, aLongName, aExplanation, aNargs),
	lCanonical(aCanonicalForm), lOptions(aForeignApplication) {
}
ForeignOption::~ForeignOption() {}

void ForeignOption::fSetMe(const char *aArg, const char * /* aSource */) {
	if (aArg != nullptr) {
		lArg = aArg;
	}
	lOptions->fRegister(this);
}
void ForeignOption::fAddArgs(std::vector<char *>& aArgv) const {
	aArgv.push_back(const_cast<char*>(lCanonical.data()));
	if (lNargs == 1) {
		aArgv.push_back(const_cast<char*>(lArg.data()));
	}
}
bool ForeignOption::fCheckRange(std::ostream& /*aLogStream*/) const {
	return true;
}
void ForeignOption::fAddToRangeFromStream(std::istream& /*aStream*/) {
}
void ForeignOption::fAddDefaultFromStream(std::istream& aStream) {
	std::getline(aStream, lArg);
}
void ForeignOption::fWriteValue(std::ostream& aStream) const {
	aStream << lArg;
}
