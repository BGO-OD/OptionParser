#ifndef __OptionsForTApplication_H__
#define __OptionsForTApplication_H__

#include "OptionsWithForeignParser.h"


class OptionsForTApplication: public ForeignApplicationOptions {
  public:
	OptionsForTApplication(const char *aName);
};


#endif
