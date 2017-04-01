#ifndef __OptionsChrono_H__
#define __OptionsChrono_H__

#include "Options.h"
#include <chrono>
#include <type_traits>

namespace options {

	template <class Rep, class Period> void OptionParseDurationString(std::chrono::duration<Rep, Period> &aDuration, const std::string& aString, int* aMonths = nullptr, int* aYears = nullptr) {
		std::stringstream sbuf(aString);
		aDuration = aDuration.zero();
		while (!sbuf.eof()) {
			double number;
			sbuf >> number;
			std::string unit;
			sbuf >> unit;
			if (unit.find("year") != std::string::npos) {
				if (aYears) {
					*aYears = number;
				} else {
					aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(std::chrono::duration<double>(31557500 * number));
				}
			} else if (unit.find("month") != std::string::npos) {
				if (aMonths) {
					*aMonths = number;
				} else {
					aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(std::chrono::duration<double>(2629800 * number));
				}
			} else if (unit.find("week") != std::string::npos) {
				aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(std::chrono::duration<double>(3600 * 24 * 7 * number));
			} else if (unit.find("day") != std::string::npos) {
				aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(std::chrono::duration<double>(3600 * 24 * number));
			} else if (unit.find("hour") != std::string::npos) {
				aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(std::chrono::duration<double>(3600 * number));
			} else if (unit.find("min") != std::string::npos) {
				aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(std::chrono::duration<double>(60 * number));
			} else if (unit.find("sec") != std::string::npos) {
				aDuration += std::chrono::duration_cast<typename std::remove_reference<decltype(aDuration)>::type>(std::chrono::duration<double>(number));
			}
		}
	};

/// template specialisation for options that are std::chrono::durations
	template <class Rep, class Period> class single<std::chrono::duration<Rep, Period>> : public base, public originalStringKeeper {
	  public:
		typedef std::chrono::duration<Rep, Period> valueType;
	  protected:
		valueType lValue;
		std::vector<valueType> lRange;
	  public:
		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, valueType aDefault = valueType::zero(), const std::vector<valueType>& aRange = {}):
			base(aShortName, aLongName, aExplanation, 1),
			lValue(aDefault) {
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
			OptionParseDurationString(value, buf);
			fAddToRange(value);
		}
		virtual void fAddDefaultFromStream(std::istream& aStream) {
			std::getline(aStream, lOriginalString);
			OptionParseDurationString(lValue, lOriginalString);
		}
		virtual void fWriteRange(std::ostream& aStream) const {
			if (! lRange.empty()) {
				aStream << "# allowed range is";
				if (lRange.size() == 2) {
					aStream << " [" << lRange[0].count() << ", " << lRange[0].count() << "]\n";
				} else {
					for (auto it = lRange.begin(); it != lRange.end(); ++it) {
						aStream << it->count() << "\n";
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
				aLogStream << fGetLongName() << " out of range (" << lValue.count() << "), must be one of:\n";
				for (auto it = lRange.begin(); it != lRange.end(); ++it) {
					aLogStream << it->count() << "\n";
				}
				return false;
			}
		}
		virtual void fWriteValue(std::ostream& aStream) const {
			aStream << lValue.count();
		}
		virtual void fSetMe(const char *aArg, const char* aSource) {
			lOriginalString = aArg;
			OptionParseDurationString(lValue, aArg);
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
	  protected:
		valueType lValue;
		std::vector<valueType> lRange;
	  public:
		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, valueType aDefault = valueType::clock::now(), const std::vector<valueType>& aRange = {}):
			base(aShortName, aLongName, aExplanation, 1),
			lValue(aDefault) {
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
					aStream << " [" << std::chrono::duration<double>(lRange[0].time_since_epoch()).count() << ", " << std::chrono::duration<double>(lRange[0].time_since_epoch()).count() << "]\n";
				} else {
					for (auto it = lRange.begin(); it != lRange.end(); ++it) {
						aStream << std::chrono::duration<double>(it->time_since_epoch()).count() << "\n";
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
					aLogStream << fGetLongName() << " out of range (" << std::chrono::duration<double>(lValue.time_since_epoch()).count() << "), must be in [" << std::chrono::duration<double>(lRange[0].time_since_epoch()).count() << ", " << std::chrono::duration<double>(lRange[0].time_since_epoch()).count() << "]\n";
					return false;
				}
			} else {
				for (auto it = lRange.begin(); it != lRange.end(); ++it) {
					if (*it == lValue) {
						return true;
					}
				}
				aLogStream << fGetLongName() << " out of range (" << std::chrono::duration<double>(lValue.time_since_epoch()).count() << "), must be one of:\n";
				for (auto it = lRange.begin(); it != lRange.end(); ++it) {
					aLogStream << std::chrono::duration<double>(it->time_since_epoch()).count() << "\n";
				}
				return false;
			}
		}
		virtual void fWriteValue(std::ostream& aStream) const {
			auto flags(aStream.flags());
			aStream << std::fixed << std::chrono::duration<double>(lValue.time_since_epoch()).count();
			aStream.flags(flags);
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
