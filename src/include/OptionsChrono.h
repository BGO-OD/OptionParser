#ifndef __OptionsChrono_H__
#define __OptionsChrono_H__

#include "Options.h"
#include <chrono>
#include <type_traits>

namespace options {
	std::chrono::duration<double> parseNumberAndUnit(std::stringstream& aStream, int* aMonths = nullptr, int* aYears = nullptr);

	template <class Rep, class Period> void parseDurationString(std::chrono::duration<Rep, Period> &aDuration, const std::string& aString, int* aMonths = nullptr, int* aYears = nullptr) {
		std::stringstream sbuf(aString);
		aDuration = aDuration.zero();
		while (!sbuf.eof()) {
			aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(parseNumberAndUnit(sbuf, aMonths, aYears));
		}
	};

/// template specialisation for options that are std::chrono::durations
	template <class Rep, class Period> class single<std::chrono::duration<Rep, Period>> : public base, public originalStringKeeper {
	  public:
		typedef std::chrono::duration<Rep, Period> valueType;
		typedef void (*valuePrinterType)(std::ostream&, valueType);
	  protected:
		valueType lValue;
		valuePrinterType lValuePrinter;
		std::vector<valueType> lRange;

	  public:
		static void fDefaultValuePrinter(std::ostream& aStream, valueType aValue) {
			auto flags(aStream.flags());
			aStream << std::fixed;
			aStream << aValue.count();
			aStream.flags(flags);
		};

		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, valueType aDefault = valueType::zero(), const std::vector<valueType>& aRange = {}, valuePrinterType aValuePrinter = fDefaultValuePrinter):
			base(aShortName, aLongName, aExplanation, 1),
			lValue(aDefault),
			lValuePrinter(aValuePrinter) {
			if (!aRange.empty()) {
				fAddToRange(aRange);
			}
		}
		virtual void fAddToRange(valueType aValue) {
			lRange.push_back(aValue);
		}
		/// add values from the iterator range [aBegin,aEnd) to the range of allowed values
		template <typename InputIt> void fAddToRange(InputIt aBegin, InputIt aEnd) {
			for (auto it = aBegin; it != aEnd; ++it) {
				fAddToRange(*it);
			}
		};
		virtual void fAddToRange(const std::vector<valueType>& aRange) {
			fAddToRange(aRange.cbegin(), aRange.cend());
		};
		virtual void fAddToRangeFromStream(std::istream& aStream) {
			std::string buf;
			std::getline(aStream, buf);
			valueType value;
			parseDurationString(value, buf);
			fAddToRange(value);
		}
		virtual void fAddDefaultFromStream(std::istream& aStream) {
			std::getline(aStream, lOriginalString);
			parseDurationString(lValue, lOriginalString);
		}
		virtual void fWriteRange(std::ostream& aStream) const {
			if (! lRange.empty()) {
				aStream << "# allowed range is";
				if (lRange.size() == 2) {
					aStream << " [";
					lValuePrinter(aStream, lRange[0]);
					aStream << ", ";
					lValuePrinter(aStream, lRange[0]);
					aStream << "]\n";
				} else {
					for (auto& rangeElement : lRange) {
						lValuePrinter(aStream, rangeElement);
						aStream << "\n";
					}
					aStream << "\n";
				}
			}

		}
		virtual bool fCheckRange(std::ostream& aLogStream) const {
			if (lRange.empty()) {
				return true;
			} else if (lRange.size() == 2) {
				if (lRange[0] <= lValue && lValue <= lRange[1]) {
					return true;
				} else {
					aLogStream << fGetLongName() << " out of range (" << lValue.count() << "), must be in [" << lRange[0].count() << ", " << lRange[1].count() << "]\n";
					return false;
				}
			} else {
				for (auto it = lRange.begin(); it != lRange.end(); ++it) {
					if (*it == lValue) {
						return true;
					}
				}
				aLogStream << fGetLongName() << " out of range (";
				lValuePrinter(aLogStream, lValue);
				aLogStream << "), must be one of:\n";
				for (auto rangeElement :  lRange) {
					lValuePrinter(aLogStream, rangeElement);
					aLogStream  << "\n";
				}
				return false;
			}
		}
		virtual void fWriteValue(std::ostream& aStream) const {
			lValuePrinter(aStream, lValue);
		}
		virtual void fSetMe(const char *aArg, const char* aSource) {
			lOriginalString = aArg;
			parseDurationString(lValue, aArg);
			fSetSource(aSource);
		}
		operator const valueType () const {
			return lValue;
		}
		const valueType fGetValue() const {
			return lValue;
		}
	};


/// template specialisation for options that are std::chrono::time_point<std::chrono::system_clock>
	template <> class single<std::chrono::system_clock::time_point> : public base, public originalStringKeeper {
	  public:
		typedef std::chrono::system_clock::time_point valueType;
		typedef void (*valuePrinterType)(std::ostream&, valueType);
	  protected:
		valueType lValue;
		valuePrinterType lValuePrinter;
		std::vector<valueType> lRange;

	  public:
		static void fDefaultValuePrinter(std::ostream& aStream, valueType aValue) {
			auto flags(aStream.flags());
			aStream << std::fixed;
			aStream << std::chrono::duration<double>(aValue.time_since_epoch()).count();
			aStream.flags(flags);
		};

		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, valueType aDefault = valueType::clock::now(), const std::vector<valueType>& aRange = {}, valuePrinterType aValuePrinter = fDefaultValuePrinter):
			base(aShortName, aLongName, aExplanation, 1),
			lValue(aDefault),
			lValuePrinter(aValuePrinter) {
			if (!aRange.empty()) {
				fAddToRange(aRange);
			}
		}

		static valueType fParseTimePointString(const std::string& aString);


		virtual void fAddToRange(valueType aValue) {
			lRange.push_back(aValue);
		}
		/// add values from the iterator range [aBegin,aEnd) to the range of allowed values
		template <typename InputIt> void fAddToRange(InputIt aBegin, InputIt aEnd) {
			for (auto it = aBegin; it != aEnd; ++it) {
				fAddToRange(*it);
			}
		};
		virtual void fAddToRange(const std::vector<valueType>& aRange) {
			fAddToRange(aRange.cbegin(), aRange.cend());
		};
		virtual void fAddToRangeFromStream(std::istream& aStream) {
			std::string buf;
			std::getline(aStream, buf);
			valueType value;
			fAddToRange(fParseTimePointString(buf));
		}
		virtual void fAddDefaultFromStream(std::istream& aStream) {
			std::getline(aStream, lOriginalString);
			lValue = fParseTimePointString(lOriginalString);
		}
		virtual void fWriteRange(std::ostream& aStream) const {
			if (! lRange.empty()) {
				aStream << "# allowed range is";
				if (lRange.size() == 2) {
					aStream << " [";
					lValuePrinter(aStream, lRange[0]);
					aStream << ", ";
					lValuePrinter(aStream, lRange[1]);
					aStream << "]\n";
				} else {
					for (auto& rangeElement : lRange) {
						lValuePrinter(aStream, rangeElement);
						aStream << "\n";
					}
					aStream << "\n";
				}
			}
		}
		virtual bool fCheckRange(std::ostream& aLogStream) const {
			if (lRange.empty()) {
				return true;
			} else if (lRange.size() == 2) {
				if (lRange[0] <= lValue && lValue <= lRange[1]) {
					return true;
				} else {
					aLogStream << fGetLongName() << " out of range (";
					lValuePrinter(aLogStream, lValue);
					aLogStream << "), must be in [";
					lValuePrinter(aLogStream, lRange[0]);
					aLogStream << ", ";
					lValuePrinter(aLogStream, lRange[1]);
					aLogStream << "]\n";
					return false;
				}
			} else {
				for (auto it = lRange.begin(); it != lRange.end(); ++it) {
					if (*it == lValue) {
						return true;
					}
				}
				aLogStream << fGetLongName() << " out of range (";
				lValuePrinter(aLogStream, lValue);
				aLogStream << "), must be one of:\n";
				for (auto& rangeElement : lRange) {
					lValuePrinter(aLogStream, rangeElement);
					aLogStream << "\n";
				}
				return false;
			}
		}
		virtual void fWriteValue(std::ostream& aStream) const {
			lValuePrinter(aStream, lValue);
		}
		virtual void fSetMe(const char *aArg, const char* aSource) {
			lOriginalString = aArg;
			lValue = fParseTimePointString(aArg);
			fSetSource(aSource);
		}
		operator const valueType () const {
			return lValue;
		}
		const valueType fGetValue() const {
			return lValue;
		}
	};

} // end of namespace options


#endif
