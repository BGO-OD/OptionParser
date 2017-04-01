#include "OptionsForTApplication.h"

namespace options {
	OptionsForTApplication::OptionsForTApplication(const char *aName) : ForeignApplicationOptions(aName) {
		new ForeignOption(this, 'b', "bForTApplication", "run in batch mode without graphics", 0, "-b");
		new ForeignOption(this, 'x', "xForTApplication", "exit on exception", 0, "-x");
		new ForeignOption(this, 'n', "nForTApplication", "do not execute logon and logoff macros as specified in .rootrc", 0, "-n");
		new ForeignOption(this, 'q', "qForTApplication", "exit after processing command line macro files", 0, "-n");
		new ForeignOption(this, 'l', "lForTApplication", "do not show splash screen", 0, "-l");

		new ForeignOption(this, '\0', "config", "print ./configure options", 0, "-config");
		new ForeignOption(this, '\0', "memstat", "run with memory usage monitoring", 0, "-memstat");
	}
} // end of namespace options
