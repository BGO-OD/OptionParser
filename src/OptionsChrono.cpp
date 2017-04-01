#include "OptionsChrono.h"
#include <ctime>
#include <iostream>
#include <algorithm>

namespace options {


//options::single<std::chrono::time_point<std::chrono::system_clock>>::valueType
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
			kNoon = 1 << 6

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
			std::cout << std::hex << "dateBits are " << dateBits << "\n";
			if (dateBits != 0) {
				timePoint = std::chrono::system_clock::now();
				if (dateBits & kDay) {
					auto coarse_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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
					std::tm broken_down_time = {};
					auto items = sscanf(pointString.c_str(), "%d/%d/%d %d:%d:%d",
					                    &(broken_down_time.tm_year), &(broken_down_time.tm_mon), &(broken_down_time.tm_mday),
					                    &(broken_down_time.tm_hour), &(broken_down_time.tm_min), &(broken_down_time.tm_sec));
					if (items >= 3) { // we have y/m/d
						broken_down_time.tm_year -= 1900;
						broken_down_time.tm_mon--;
						broken_down_time.tm_isdst = -1;
						timePoint = std::chrono::system_clock::from_time_t(std::mktime(&broken_down_time));
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
			OptionParseDurationString(offset, offsetString, &months, &years);

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

} // end of namespace options
