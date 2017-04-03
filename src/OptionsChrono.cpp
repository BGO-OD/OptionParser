#include "OptionsChrono.h"
#include <ctime>
#include <iostream>
#include <algorithm>

namespace options {
	namespace internal {

		std::chrono::duration<double> parseNumberAndUnit(std::stringstream& aStream, int* aMonths, int* aYears) {
			class unitDescriptor {
			  public:
				enum unitType {
					kUnspecific = 0,
					kWithEnlargingPrefix = 1 << 0,
					kWithDiminishingPrefix = 1 << 1,
					kWithPrefix = kWithEnlargingPrefix | kWithDiminishingPrefix,
					kMonth = 1 << 2,
					kYear = (1 << 3) | kWithEnlargingPrefix ,
					kSecond = kWithDiminishingPrefix
				};
				const char *lUnitName;
				double lFactor;
				unitType lType;
				unitDescriptor(decltype(lUnitName) aUnitName,
				               decltype(lFactor) aFactor,
				               decltype(lType) aType):
					lUnitName(aUnitName), lFactor(aFactor), lType(aType) {
				};
			};

			static std::vector<unitDescriptor> unitDescriptors({
				{"year",     31557500, unitDescriptor::kYear},
				{"yr",       31557500, unitDescriptor::kYear},
				{"a",        31557500, unitDescriptor::kYear},
				{"month",     2629800, unitDescriptor::kMonth},
				{"week", 3600 * 24 * 7, unitDescriptor::kUnspecific},
				{"day",     3600 * 24, unitDescriptor::kUnspecific},
				{"hour",         3600, unitDescriptor::kUnspecific},
				{"minute",         60, unitDescriptor::kUnspecific},
				{"min",            60, unitDescriptor::kUnspecific},
				{"second",          1, unitDescriptor::kSecond},
				{"sec",             1, unitDescriptor::kSecond},
				{"s",               1, unitDescriptor::kSecond}
			}
			                                                  );

			class prefixDescriptor {
			  public:
				const char *lName;
				long long lFactor;
				std::string::size_type lNameLength;
				prefixDescriptor(decltype(lName) aName, decltype(lFactor) aFactor) :
					lName(aName), lFactor(aFactor) {
					lNameLength = strlen(aName);
				}
			};
			static std::vector<prefixDescriptor> diminishingPrefixDescriptors ({
				{"milli",      1000},
				{"m",          1000},
				{"micro",   1000000},
				{"u",       1000000},
				{"nano", 1000000000},
				{"n",    1000000000}
			}
			                                                                  );
			static std::vector<prefixDescriptor> enlargingPrefixDescriptors ({
				{"kilo",      1000},
				{"k",         1000},
				{"mega",   1000000},
				{"m",      1000000},
				{"giga", 1000000000},
				{"g",    1000000000}
			}
			                                                                );

			double number;
			aStream >> number;
			if (aStream.eof()) {
				return std::chrono::duration<double>::zero();
			}
			std::string unit;
			aStream >> unit;
			std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
			for (auto& unitDesc : unitDescriptors) {
				auto location = unit.find(unitDesc.lUnitName);
				if (location != std::string::npos) {
					auto unitNameLength = strlen(unitDesc.lUnitName);
					auto nExtraChars = unit.size() - location - unitNameLength;
					if (nExtraChars > 0 && // next line is special condition to allow a plural-s
					        !(nExtraChars == 1 && unitNameLength > 2 && unit.back() == 's')) {
						parser::fGetInstance()->fGetErrorStream() << "Garbage in '" << unit << "' after the unit '" << unitDesc.lUnitName << "'\n";
						parser::fGetInstance()->fComplainAndLeave();
					}
					if (location != 0) {
						if (!(unitDesc.lType & unitDescriptor::kWithPrefix)) {
							parser::fGetInstance()->fGetErrorStream() << "Garbage in '" << unit << "' before the unit '" << unitDesc.lUnitName << "'\n";
							parser::fGetInstance()->fComplainAndLeave();
						}
						bool prefixFound = false;
						if (unitDesc.lType & unitDescriptor::kWithDiminishingPrefix) {
							for (auto& prefix : diminishingPrefixDescriptors) {
								if (unit.compare(0, location, prefix.lName) == 0) {
									number /= prefix.lFactor;
									prefixFound = true;
									break;
								}
							}
						}
						if (unitDesc.lType & unitDescriptor::kWithEnlargingPrefix) {
							for (auto& prefix : enlargingPrefixDescriptors) {
								if (unit.compare(0, location, prefix.lName) == 0) {
									number *= prefix.lFactor;
									prefixFound = true;
									break;
								}
							}
						}
						if (!prefixFound) {
							parser::fGetInstance()->fGetErrorStream() << "No valid prefix found in '" << unit << "'\n";
							parser::fGetInstance()->fComplainAndLeave();
						}
					}
					if ((unitDesc.lType == unitDescriptor::kYear && aYears) ||
					        (unitDesc.lType == unitDescriptor::kMonth && aMonths)) {
						if (unitDesc.lType == unitDescriptor::kYear) {
							*aYears = number;
						} else {
							*aMonths = number;
						}
						return  std::chrono::duration<double>::zero();
					} else {
						return std::chrono::duration<double>(unitDesc.lFactor * number);
					}
				}
			}
			parser::fGetInstance()->fGetErrorStream() << "Unrecognized time unit in '" << unit << "'\n";
			parser::fGetInstance()->fComplainAndLeave();
			return std::chrono::duration<double>::zero();
		}
	} // end of namespace internal


	std::chrono::system_clock::time_point options::single<std::chrono::system_clock::time_point>::fParseTimePointString(const std::string& aString) {
		std::string::size_type pointStringStart = 0;
		std::string::size_type pointStringStop = std::string::npos;
		std::string::size_type offsetStringStart = std::string::npos;
		std::string::size_type offsetStringStop = std::string::npos;
		bool offsetIsNegative = false;
		{
			auto after = aString.find("after");
			if (after == 0) { // we have no time point
				offsetStringStart = 6; // the offset starts after after
				pointStringStart = std::string::npos;
			} else if (after != std::string::npos) { // "after" after the offset before the point
				offsetStringStart = 0;
				offsetStringStop = after;
				pointStringStart = after + 6;
			}
		}
		{
			auto before = aString.find("before");
			if (before == 0) { // we have no time point
				offsetStringStart = 6; // the offset starts after before
				pointStringStart = std::string::npos;
				offsetIsNegative = true;
			} else if (before != std::string::npos) { // "before" after the offset before the point
				offsetStringStart = 0;
				offsetStringStop = before;
				pointStringStart = before + 7;
				offsetIsNegative = true;
			}
		}

		valueType timePoint;
		enum dateBitType {
			kNow = 1 << 0,
			kToday = 1 << 1,
			kTomorrow = 1 << 2,
			kYesterday = 1 << 3,
			kWeekday = 1 << 4,
			kDay = kToday | kTomorrow | kYesterday | kWeekday,
			kLast = 1 << 5,
			kThis = 1 << 6,
			kNoon = 1 << 7

		};
		if (pointStringStart < aString.size()) {
			auto pointString = aString.substr(pointStringStart, pointStringStop);
			std::transform(pointString.begin(), pointString.end(), pointString.begin(), ::tolower);

			typename std::underlying_type<dateBitType>::type dateBits = 0;
			int weekDay = 0;
			if (pointString.find("now") != std::string::npos) {
				dateBits |= kNow;
			} else if (pointString.find("today") != std::string::npos) {
				dateBits |= kToday;
			} else if (pointString.find("yesterday") != std::string::npos) {
				dateBits |= kYesterday;
			} else if (pointString.find("tomorrow") != std::string::npos) {
				dateBits |= kTomorrow;
			} else if (pointString.find("sun") != std::string::npos) {
				dateBits |= kWeekday;
				weekDay = 0;
			} else if (pointString.find("mon") != std::string::npos) {
				dateBits |= kWeekday;
				weekDay = 1;
			} else if (pointString.find("tue") != std::string::npos) {
				dateBits |= kWeekday;
				weekDay = 2;
			} else if (pointString.find("wed") != std::string::npos) {
				dateBits |= kWeekday;
				weekDay = 3;
			} else if (pointString.find("thu") != std::string::npos) {
				dateBits |= kWeekday;
				weekDay = 4;
			} else if (pointString.find("fri") != std::string::npos) {
				dateBits |= kWeekday;
				weekDay = 5;
			} else if (pointString.find("sat") != std::string::npos) {
				dateBits |= kWeekday;
				weekDay = 6;
			}
			if (pointString.find("noon") != std::string::npos) {
				dateBits |= kNoon;
			}
			if (pointString.find("last") != std::string::npos) {
				dateBits |= kLast;
			}
			if (pointString.find("this") != std::string::npos) {
				dateBits |= kThis;
			}

			if (dateBits != 0) {
				timePoint = std::chrono::system_clock::now();
				if (dateBits & kDay) {
					auto coarse_time = std::chrono::system_clock::to_time_t(timePoint);
					auto broken_down_time = std::localtime(&coarse_time);

					broken_down_time->tm_sec = 0;
					broken_down_time->tm_min = 0;
					broken_down_time->tm_hour = (dateBits & kNoon) ? 12 : 0;
					if (dateBits & kYesterday) {
						broken_down_time->tm_mday--;
					} else if (dateBits & kTomorrow) {
						broken_down_time->tm_mday++;
					} else if (dateBits & kWeekday) {
						auto dayOffset = weekDay - broken_down_time->tm_wday;
						if (dateBits & kLast) {
							if (dayOffset >= 0) {
								dayOffset -= 7;
							}
						} else if (dateBits & kThis) {
							// no change to the Offset
						} else { // we imply the next day of that name
							if (dayOffset <= 0) {
								dayOffset += 7;
							}
						}
						broken_down_time->tm_mday += dayOffset;
					}

					timePoint = std::chrono::system_clock::from_time_t(std::mktime(broken_down_time));
				}
			} else { // no date bits found, we have a direct specification
				if (pointString[0] == '@') { // as for date(1) this is seconds since 1970
					auto seconds = std::stod(pointString.substr(1));
					timePoint =  std::chrono::system_clock::from_time_t(0) +
					             std::chrono::duration_cast<valueType::duration>(std::chrono::duration<double>(seconds));
				} else {
					std::tm broken_down_time;
					memset(&broken_down_time, 0, sizeof(broken_down_time));
					auto items = sscanf(pointString.c_str(), "%d/%d/%d %d:%d:%d",
					                    &(broken_down_time.tm_year), &(broken_down_time.tm_mon), &(broken_down_time.tm_mday),
					                    &(broken_down_time.tm_hour), &(broken_down_time.tm_min), &(broken_down_time.tm_sec));
					if (items >= 3) { // we have y/m/d
						broken_down_time.tm_year -= 1900;
						broken_down_time.tm_mon--;
						broken_down_time.tm_isdst = -1;
						timePoint = std::chrono::system_clock::from_time_t(std::mktime(&broken_down_time));
					} else {
						parser::fGetInstance()->fGetErrorStream() << "Unrecognized time in '" << pointString << "'\n";
						parser::fGetInstance()->fComplainAndLeave();
					}
				}
			}
		} else {
			timePoint = std::chrono::system_clock::now();
		}

		if (offsetStringStart < aString.size()) {
			valueType::duration offset;
			auto offsetString = aString.substr(offsetStringStart, offsetStringStop);
			std::transform(offsetString.begin(), offsetString.end(), offsetString.begin(), ::tolower);

			int months = 0;
			int years = 0;
			internal::parseDurationString(offset, offsetString, &months, &years);

			if (offsetIsNegative) {
				timePoint -= offset;
			} else {
				timePoint += offset;
			}
			if (months != 0 || years != 0) {
				auto coarse_time = std::chrono::system_clock::to_time_t(timePoint);
				auto fractionalPart = timePoint - std::chrono::system_clock::from_time_t(coarse_time);
				auto broken_down_time = std::localtime(&coarse_time);
				if (offsetIsNegative) {
					broken_down_time->tm_mon -= months;
					broken_down_time->tm_year -= years;
				} else {
					broken_down_time->tm_mon += months;
					broken_down_time->tm_year += years;
				}
				timePoint = std::chrono::system_clock::from_time_t(std::mktime(broken_down_time));
				timePoint += fractionalPart;
			}
		}
		return timePoint;
	}

	void options::single<std::chrono::system_clock::time_point>::fDefaultValuePrinter(std::ostream& aStream, valueType aValue) {
		auto flags(aStream.flags());
		aStream << std::fixed;
		aStream << std::chrono::duration<double>(aValue.time_since_epoch()).count();
		aStream.flags(flags);
	}
	options::single<std::chrono::system_clock::time_point>::single(char aShortName, const std::string& aLongName, const std::string& aExplanation, valueType aDefault, const std::vector<valueType>& aRange, valuePrinterType aValuePrinter):
		base(aShortName, aLongName, aExplanation, 1),
		lValue(aDefault),
		lValuePrinter(aValuePrinter) {
		if (!aRange.empty()) {
			fAddToRange(aRange);
		}
	}
	options::single<std::chrono::system_clock::time_point>::single(char aShortName, const std::string& aLongName, const std::string& aExplanation, const std::string& aDefault, const std::vector<std::string>& aRange, valuePrinterType aValuePrinter):
		base(aShortName, aLongName, aExplanation, 1),
		lValuePrinter(aValuePrinter) {
		lValue = fParseTimePointString(aDefault);
		lOriginalString = aDefault;
		if (!aRange.empty()) {
			fAddToRange(aRange);
		}
	}

	void options::single<std::chrono::system_clock::time_point>::fSetValuePrinter(valuePrinterType aValuePrinter) {
		lValuePrinter = aValuePrinter;
	}


	void options::single<std::chrono::system_clock::time_point>::fAddToRange(valueType aValue) {
		lRange.push_back(aValue);
	}
	void options::single<std::chrono::system_clock::time_point>::fAddToRange(const std::string& aString) {
		fAddToRange(fParseTimePointString(aString));
	}

	void options::single<std::chrono::system_clock::time_point>::fAddToRange(const std::vector<valueType>& aRange) {
		fAddToRange(aRange.cbegin(), aRange.cend());
	};
	void options::single<std::chrono::system_clock::time_point>::fAddToRange(const std::vector<std::string>& aRange) {
		fAddToRange(aRange.cbegin(), aRange.cend());
	};
	void options::single<std::chrono::system_clock::time_point>::fAddToRangeFromStream(std::istream& aStream) {
		std::string buf;
		std::getline(aStream, buf);
		fAddToRange(buf);
	}
	void options::single<std::chrono::system_clock::time_point>::fAddDefaultFromStream(std::istream& aStream) {
		std::getline(aStream, lOriginalString);
		lValue = fParseTimePointString(lOriginalString);
	}

	void options::single<std::chrono::system_clock::time_point>::fWriteRange(std::ostream& aStream) const {
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
	bool options::single<std::chrono::system_clock::time_point>::fCheckRange(std::ostream& aLogStream) const {
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
	void options::single<std::chrono::system_clock::time_point>::fWriteValue(std::ostream& aStream) const {
		lValuePrinter(aStream, lValue);
	}
	void options::single<std::chrono::system_clock::time_point>::fSetMe(const char *aArg, const char* aSource) {
		lOriginalString = aArg;
		lValue = fParseTimePointString(aArg);
		fSetSource(aSource);
	}
	const options::single<std::chrono::system_clock::time_point>::valueType options::single<std::chrono::system_clock::time_point>::fGetValue() const {
		return lValue;
	}

} // end of namespace options
