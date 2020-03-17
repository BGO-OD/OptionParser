#ifndef __OptionsRegex_H__
#define __OptionsRegex_H__

#include <regex>
#include <Options.h>

namespace options {
	template <> class single<std::regex>:
		public std::regex,
		public internal::typed_base<std::regex, true>,
		public originalStringKeeper {
		typedef std::string rangeValueType;
		std::regex::flag_type regexOption;
	  public:
		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, const std::string& aDefault = "", std::regex::flag_type aRegexOption = std::regex::ECMAScript) :
			internal::typed_base<std::regex, true>(aShortName, aLongName, aExplanation, 1),
			regexOption(aRegexOption) {
			if (! aDefault.empty() ) {
				assign(aDefault, std::regex::optimize | regexOption);
			}
			lOriginalString = aDefault;
		}
		void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) override {
			using escapedIO::operator>>;
			aStream >> lOriginalString;
			assign(lOriginalString, std::regex::optimize | regexOption);
			fSetSource(aSource);
		}
		void fCheckRange() const override {
			return;
		}
		void fAddDefaultFromStream(std::istream& aStream) override {
			std::getline(aStream, lOriginalString);
			assign(lOriginalString);
		}
		void fWriteValue(std::ostream& aStream) const override {
			aStream << lOriginalString;
		}
	};

}

#endif
