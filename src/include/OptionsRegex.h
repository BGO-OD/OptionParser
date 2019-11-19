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
	  public:
		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, const std::string& aDefault = "") :
			internal::typed_base<std::regex, true>(aShortName, aLongName, aExplanation, 1) {
			if (! aDefault.empty() ) {
				std::regex& dieses(*this);
				dieses = aDefault;
			}
			lOriginalString = aDefault;
		}
		void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) override {
			using escapedIO::operator>>;
			aStream >> lOriginalString;
			static_cast<std::regex*>(this)->assign(lOriginalString);
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
