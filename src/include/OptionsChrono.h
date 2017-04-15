#ifndef __OptionsChrono_H__
#define __OptionsChrono_H__

#ifdef __Options_H__
#error When using OptionsChrono.h do not include Options.h, at least not before!
#endif

#include <iostream>
#include <chrono>
#include <type_traits>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace options {
/// template specialisation for options that are std::chrono::time_point<std::chrono::system_clock>
	namespace internal {
		std::chrono::duration<double> parseNumberAndUnit(std::stringstream& aStream, int* aMonths = nullptr, int* aYears = nullptr);
		std::chrono::system_clock::time_point fParseTimePointString(const std::string& aString);

		/// parse a string into a std::chrono::duration, if given set the years and months separately
		template <class Rep, class Period> void parseDurationString(std::chrono::duration<Rep, Period> &aDuration, const std::string& aString, int* aMonths = nullptr, int* aYears = nullptr) {
			auto between = aString.find("between");
			if (between != std::string::npos) {
				auto andpos = aString.find("and");
				if (andpos != std::string::npos) {
					auto t0 = fParseTimePointString(aString.substr(between + 8, andpos - 1 - (between + 8) ));
					auto t1 = fParseTimePointString(aString.substr(andpos + 4));
					aDuration = std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(t1 - t0);
				} else {
					throw std::runtime_error("duration with 'between' without and");
				}
			} else {
				std::stringstream sbuf(aString);
				aDuration = aDuration.zero();
				while (!sbuf.eof()) {
					aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(parseNumberAndUnit(sbuf, aMonths, aYears));
				}
			}
		};
	} // end of namespace internal
	namespace escapedIO {
		std::ostream& operator<<(std::ostream& aStream, const std::chrono::system_clock::time_point& aTime);
		std::istream& operator>>(std::istream& aStream, std::chrono::system_clock::time_point& aTime);

		template <class Rep, class Period> std::ostream& operator<<(std::ostream& aStream, const std::chrono::duration<Rep, Period>& aDuration) {
			auto flags(aStream.flags());
			aStream << std::fixed;
			aStream << aDuration.count();
			aStream.flags(flags);
			return aStream;
		}
		template <class Rep, class Period> std::istream& operator>>(std::istream& aStream, std::chrono::duration<Rep, Period>& aDuration) {
			std::string buf;
			aStream >> buf;
			if (!aStream.fail()) {
				internal::parseDurationString(aDuration, buf);
			}
			return aStream;
		};
	} // end of namespace escapedIO
} // end of namespace options

#include "Options.h"


namespace options {
	template <> class single<std::chrono::system_clock::time_point> :
		public std::chrono::system_clock::time_point,
		public internal::typed_base<std::chrono::system_clock::time_point>,
		public originalStringKeeper {
	  public:
		typedef rangeValueType valueType;
		typedef void (*valuePrinterType)(std::ostream&, valueType);
	  protected:
		valuePrinterType lValuePrinter;
		std::vector<valueType> lRange;

	  public:
		static void fDefaultValuePrinter(std::ostream& aStream, valueType aValue);

		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, valueType aDefault = valueType::clock::now(), const std::vector<valueType>& aRange = {}, valuePrinterType aValuePrinter = fDefaultValuePrinter);
		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, const std::string& aDefault, const std::vector<std::string>& aRange = {}, valuePrinterType aValuePrinter = fDefaultValuePrinter);

		void fSetValuePrinter(valuePrinterType aValuePrinter);

		void fAddDefaultFromStream(std::istream& aStream) override;
		void fWriteRange(std::ostream& aStream) const override;
		void fWriteValue(std::ostream& aStream) const override;
		void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) override;
		bool fCheckRange(std::ostream& aLogStream) const override {
			return this->fCheckValueForRange(*this, aLogStream);
		}
	};






/// template specialisation for options that are std::chrono::durations
	template <class Rep, class Period> class single<std::chrono::duration<Rep, Period>> :
		        public std::chrono::duration<Rep, Period>,
		        public internal::typed_base<std::chrono::duration<Rep, Period>>,
		        public originalStringKeeper {
	  public:
		typedef std::chrono::duration<Rep, Period> valueType; // why does the one in typed_base not work?
		typedef void (*valuePrinterType)(std::ostream&, valueType);
	  protected:
		valuePrinterType lValuePrinter;
		std::vector<valueType> lRange;

	  public:
		static void fDefaultValuePrinter(std::ostream& aStream, valueType aValue) {
			using escapedIO::operator<<;
			aStream << aValue;
		};


		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, valueType aDefault = valueType::zero(), const std::vector<valueType>& aRange = {}, valuePrinterType aValuePrinter = fDefaultValuePrinter):
			valueType(aDefault),
			internal::typed_base<valueType>(aShortName, aLongName, aExplanation, 1),
			lValuePrinter(aValuePrinter) {
			if (!aRange.empty()) {
				this->fAddToRange(aRange);
			}
		}
		void fAddDefaultFromStream(std::istream& aStream) override {
			std::getline(aStream, lOriginalString);
			internal::parseDurationString(*this, lOriginalString);
		}
		void fWriteValue(std::ostream& aStream) const override {
			lValuePrinter(aStream, *this);
		}
		void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) override {
			using escapedIO::operator>>;
			aStream >> lOriginalString;
			internal::parseDurationString(*this, lOriginalString);
			this->fSetSource(aSource);
		}
		bool fCheckRange(std::ostream& aLogStream) const override {
			return this->fCheckValueForRange(*this, aLogStream);
		}
	};



} // end of namespace options


#endif
