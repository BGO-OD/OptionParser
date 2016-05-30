#include "Options.h"
#include <list>

int main(int argc, const char *argv[]) {
	Option<int> number('n', "number", "some number", 0);
	Option<int> number2('k', "kumber", "some other number", 0, {0, 3, 3});
	OptionMap<int, std::vector<std::pair<std::string, int> > > numbers('m', "numbers", "several numbers");
	OptionMap<std::string, std::list<std::pair<std::string, std::string> > > strings('s', "strings", "several strings");
	Option<const char *>cs('c', "cstring", "a c string", nullptr, {"bla", "blubb"});
	Option<std::string>Cs('C', "Cstring", "a C++ string", "",  {"bla", "blubb"});
	OptionContainer<double> dnums('d', "doubles", "double numbers");
	OptionContainer<const char*, std::list<const char*>> stringl('S', "listString", "list of strings");
	OptionContainer<std::string, std::list<std::string>> stringS('x', "liststring", "list of std::strings");
	Option<double> complexDescription('\0', "ComplexDescription", "Pass here the Bremsstrahl-Tagging-Hodoscope-Engineering-Assemply-Rate in Hz", 84.);
	Option<double> moreComplexDescription('\0', "MoreComplexDescription", "very complicated example option with very long explanation to illustrate automatic wrapping in help output when the explanations become very long and would break readability otherwise.", 42.);
	Option<double> evenMoreComplexDescription('\0', "EvenMoreComplexDescription", "very complicated example option with very long explanation containing averylongwordwhichisunbreakableandthustriggersforcefulwordwrappinginaninconvenientplacetokeepthingssomehowatleastabitreadable.", 21.);

	cs.fForbid(&Cs);
	Cs.fForbid(&cs);

	OptionParser parser("option parsing example");
	auto unusedOptions = parser.fParse(argc, argv);

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
		std::cout << " double number '" << dnum << "\n";
	}
	for (auto & it : stringl) {
		std::cout << " list string '" << it << "\n";
	}
	for (auto & it : stringS) {
		std::cout << " list std::string '" << it << "\n";
	}
	return number;
}
