#ifndef __OptionsForTApplication_H__
#define __OptionsForTApplication_H__

#include "OptionsWithForeignParser.h"

namespace options {


/// specific class to give the options for a TApplication as ForeignApplicationOptions to the option parser
class OptionsForTApplication: public ForeignApplicationOptions {
  public:
	OptionsForTApplication(const char *aName);
};

} // end of namespace options

#endif
