#ifndef __OptionsForTApplication_H__
#define __OptionsForTApplication_H__

#include "OptionsWithForeignParser.h"


/// specific class to give the options for a TApplication as ForeignApplicationOptions to the option parser
class OptionsForTApplication: public ForeignApplicationOptions {
  public:
	OptionsForTApplication(const char *aName);
};


#endif
