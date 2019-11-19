#include "OptionsChrono.h"
#include <ctime>
#include <algorithm>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>


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

		bool fParseStreamToBrokenDownTime(std::istream &aStream, std::tm* aBrokenDownTime, double& aFractional, std::string& timezone) {
			aStream >> aBrokenDownTime->tm_year;
			if (aStream.fail() || aStream.get() != '/') {
				return false;
			}
			aStream >> aBrokenDownTime->tm_mon;
			if (aStream.fail() || aStream.get() != '/') {
				return false;
			}
			aStream >> aBrokenDownTime->tm_mday;
			if (aStream.fail()) {
				return false;
			}
			aStream >> aBrokenDownTime->tm_hour;
			if (!aStream.fail()) {
				if (aStream.get() != ':') {
					return false;
				}
				aStream >> aBrokenDownTime->tm_min;
				if (aStream.fail()) {
					return false;
				}
				if (aStream.peek() == ':') {
					aStream.get();
					aStream >> aFractional;
					aBrokenDownTime->tm_sec = aFractional;
					aFractional -= aBrokenDownTime->tm_sec;
					if (aStream.fail()) {
						return false;
					}
				}
			}
			aStream >> timezone;

			return true;
		}

		std::chrono::system_clock::time_point fParseTimePointString(const std::string& aString) {
			std::string::size_type pointStringStart = 0;
			std::string::size_type pointStringLength = std::string::npos;
			std::string::size_type offsetStringStart = std::string::npos;
			std::string::size_type offsetStringLength = std::string::npos;
			bool offsetIsNegative = false;
			{
				auto after = aString.find("after");
				if (after == 0) { // we have no time point
					offsetStringStart = 6; // the offset starts after after
					pointStringStart = std::string::npos;
				} else if (after != std::string::npos) { // "after" after the offset before the point
					offsetStringStart = 0;
					offsetStringLength = after - 1;
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
					offsetStringLength = before - 1;
					pointStringStart = before + 7;
					offsetIsNegative = true;
				}
			}

			std::chrono::system_clock::time_point timePoint;
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
				auto pointString = aString.substr(pointStringStart, pointStringLength);
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
						             std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<double>(seconds));
					} else { // try direct spec like yyy/mm/dd [HH:MM:SS]
						std::tm broken_down_time;
						memset(&broken_down_time, 0, sizeof(broken_down_time));
						double fractionalPart;
						std::stringstream buf(aString.substr(pointStringStart, pointStringLength));
						std::string timezone;
						if (internal::fParseStreamToBrokenDownTime(buf, &broken_down_time, fractionalPart, timezone)) {
							broken_down_time.tm_year -= 1900;
							broken_down_time.tm_mon--;
							broken_down_time.tm_isdst = -1;
							#ifdef TZFILE_PATH
							auto oldTZ = getenv("TZ");
							std::string oldTZstring;
							if (oldTZ) {
								oldTZstring = oldTZ;
							}
							if (timezone.length() > 2) {
								std::string tzfilename(TZFILE_PATH);
								tzfilename += timezone;
								struct stat s;
								if (stat(tzfilename.c_str(), &s) < 0) {
									parser::fGetInstance()->fGetErrorStream() << "can't find timezone file '" << tzfilename << "'\n";
									parser::fGetInstance()->fComplainAndLeave();
								}
								timezone = ":" + timezone;
								setenv("TZ", timezone.c_str(), true);
								tzset();
							}
							#endif
							timePoint = std::chrono::system_clock::from_time_t(std::mktime(&broken_down_time));
							timePoint += std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<double>(fractionalPart));
							#ifdef TZFILE_PATH
							if (timezone.length() > 2) {
								if (oldTZ) {
									setenv("TZ", oldTZstring.c_str(), true);
								} else {
									unsetenv("TZ");
								}
							}
							#endif
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
				std::chrono::system_clock::duration offset;
				auto offsetString = aString.substr(offsetStringStart, offsetStringLength);
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
	} // end of namespace internal


	void single<std::chrono::system_clock::time_point>::fDefaultValuePrinter(std::ostream& aStream, const valueType& aValue) {
		auto flags(aStream.flags());
		aStream << std::fixed;
		aStream << std::chrono::duration<double>(aValue.time_since_epoch()).count();
		aStream.flags(flags);
	}
	single<std::chrono::system_clock::time_point>::single(char aShortName, const std::string& aLongName, const std::string& aExplanation, valueType aDefault, const std::vector<valueType>& aRange, valuePrinterType aValuePrinter):
		internal::typed_base<std::chrono::system_clock::time_point>(aShortName, aLongName, aExplanation, 1),
		valuePrinter(aValuePrinter) {
		*static_cast<valueType*>(this) = aDefault;
		if (!aRange.empty()) {
			fAddToRange(aRange);
		}
	}
	single<std::chrono::system_clock::time_point>::single(char aShortName, const std::string& aLongName, const std::string& aExplanation, const std::string& aDefault, const std::vector<std::string>& aRange, valuePrinterType aValuePrinter):
		internal::typed_base<std::chrono::system_clock::time_point>(aShortName, aLongName, aExplanation, 1),
		valuePrinter(aValuePrinter) {
		try {
			*static_cast<valueType*>(this) = internal::fParseTimePointString(aDefault);
		} catch (const std::runtime_error& e) {
			throw internal::optionError(this, e.what());
		}
		lOriginalString = aDefault;
		if (!aRange.empty()) {
			fAddToRange(aRange);
		}
	}

	void single<std::chrono::system_clock::time_point>::fAddDefaultFromStream(std::istream& aStream) {
		std::getline(aStream, lOriginalString);
		try {
			*static_cast<valueType*>(this) = internal::fParseTimePointString(lOriginalString);
		} catch (const std::runtime_error& e) {
			throw internal::optionError(this, e.what());
		}
	}

	void single<std::chrono::system_clock::time_point>::fWriteRange(std::ostream& aStream) const {
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

	void single<std::chrono::system_clock::time_point>::fWriteValue(std::ostream& aStream) const {
		lValuePrinter(aStream, *this);
	}
	void single<std::chrono::system_clock::time_point>::fSetMe(std::istream& aStream, const internal::sourceItem& aSource) {
		using escapedIO::operator>>;
		aStream >> lOriginalString;
		try {
			*static_cast<valueType*>(this) = internal::fParseTimePointString(lOriginalString);
		} catch (const std::runtime_error& e) {
			throw internal::optionError(this, e.what());
		}
		fSetSource(aSource);
	}


	namespace escapedIO {
		std::ostream& operator<<(std::ostream& aStream, const std::chrono::system_clock::time_point& aTime) {
			single<std::chrono::system_clock::time_point>::fDefaultValuePrinter(aStream, aTime);
			return aStream;
		}
		std::istream& operator>>(std::istream& aStream, std::chrono::system_clock::time_point& aTime) {
			std::string buf;
			aStream >> buf;
			if (!aStream.fail()) {
				aTime = internal::fParseTimePointString(buf);
			}
			return aStream;
		}

	} // end of namespace escapedIO

} // end of namespace options
