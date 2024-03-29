#include "OptionsChrono.h"
#include <list>
#include "OptionsForTApplication.h"


int main(int argc, const char *argv[]) {
	options::single<int> number('N', "number", "some number", 0);
	options::single<int> number2('k', "kumber", "some other number", 0, {0, 3, 3});
	options::map<int, std::vector<std::pair<std::string, int> > > numbers('m', "numbers", "several numbers");
	options::map<std::string, std::list<std::pair<std::string, std::string> > > strings('s', "strings", "several strings");
	options::single<const char *>cs('c', "cstring", "a c string", "", {"bla", "blubb"});
	options::single<std::string>Cs('C', "Cstring", "a C++ string", "",  {"bla", "blubb"});
	options::container<double> dnums('d', "doubles", "double numbers");
	//	options::container<const char*, std::list<const char*>> stringl('S', "listString", "list of strings");
	options::container<std::string, std::list<std::string>> stringS('X', "liststring", "list of std::strings");
	options::single<double> complexDescription('\0', "ComplexDescription", "Pass here the Bremsstrahl-Tagging-Hodoscope-Engineering-Assemply-Rate in Hz", 84.);
	options::single<double> moreComplexDescription('\0', "MoreComplexDescription", "very complicated example option with very long explanation to illustrate automatic wrapping in help output when the explanations become very long and would break readability otherwise.", 42.);
	options::single<double> evenMoreComplexDescription('\0', "EvenMoreComplexDescription", "very complicated example option with very long explanation containing averylongwordwhichisunbreakableandthustriggersforcefulwordwrappinginaninconvenientplacetokeepthingssomehowatleastabitreadable.", 21.);

	options::single<options::postFixedNumber<size_t>> size('\0', "size", "a size");
	options::single<options::postFixedNumber<ssize_t>> ssize('\0', "ssize", "a ssize");
	options::single<options::postFixedNumber<double>> dpost('\0', "dpost", "double postfiexd");
	options::single<std::chrono::system_clock::time_point> date('\0', "date", "a date");
	options::single<std::chrono::duration<long>> dur('\0', "dur", "a duration");
	options::single<bool> lateOption('\0', "lateOption", "try to book an option late", false);
	cs.fForbid(&Cs);
	Cs.fForbid(&cs);

	options::container<std::chrono::system_clock::time_point> dates('\0', "dates", "list of dates");

	options::OptionsForTApplication TApplicationOptions(argv[0]);

	options::positional<options::single<float>>posNumber(10, "posnumber", "positional float number", 0);
	options::positional<options::container<std::string>>files(20, "files", "positional file list");
	options::positional<options::single<const char *>>dest(30, "dest", "positional destination file", "");

	options::parser parser("option parsing example");

	parser.fRequire(&number);

	auto unusedOptions = parser.fParse(argc, argv);

	TApplicationOptions.fFinalize(unusedOptions.begin(), unusedOptions.end());

	for (auto & unusedOption : unusedOptions) {
		std::cout << "unused option : '" << unusedOption << "'" << std::endl;
	}
	for (auto & num : numbers) {
		std::cout << " number '" << num.first << "' is '" << num.second << "'\n";
	}
	for (auto & str : strings) {
		std::cout << " string '" << str.first << "' is '" << str.second << "'\n";
	}
	for (double & dnum : dnums) {
		std::cout << " double number '" << dnum << "'\n";
	}
	//	for (auto & it : stringl) {
	//	std::cout << " list string '" << it << "'\n";
	//}
	for (auto & it : stringS) {
		std::cout << " list std::string '" << it << "'\n";
	}

	std::cout << "and the time variable is:";
	date.fWriteValue(std::cout);
	auto timebuf = std::chrono::system_clock::to_time_t(date);
	std::cout << ", that is " << std::ctime(&timebuf);

	std::cout << "the duration is: ";
	dur.fWriteValue(std::cout);
	std::cout << " or " << std::chrono::duration_cast<std::chrono::hours>(dur).count()  << " hours \n";

	if (lateOption) {
		options::single<bool> optionLate('\0', "lateOptionTest", "option booked late", false);
	}

	if (size.fIsSet()) {
		std::cout << "size is " << size << "\n";
	}
	if (ssize.fIsSet()) {
		std::cout << "ssize is " << ssize << "\n";
	}
	if (dpost.fIsSet()) {
		std::cout << "dpost is " << dpost << "\n";
	}

	return number;
}
