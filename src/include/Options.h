/*
   Option parser  C++(11) library for parsing command line options
    Copyright (C) 2016  Juergen Hannappel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __Options_H__
#define __Options_H__

#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <typeinfo>

namespace options {
	namespace internal {
		/// class to remember the file (or cmd line) an option was set from
		class sourceFile {
		  protected:
			std::string lName;
			const sourceFile& lParent;
		  public:
			static const sourceFile gUnsetSource;
			static const sourceFile gCmdLine;
			sourceFile(const std::string& aName, decltype(lParent) aParent):
				lName(aName),
				lParent(aParent) {
			};
			const std::string& fGetName() const {
				return lName;
			};
		};

		/// class to remember from which line (or item) of a file/line an option was set from
		class sourceItem {
		  protected:
			const sourceFile* lFile;
			int lLineNumber;
		  public:
			sourceItem(): lFile(&sourceFile::gUnsetSource) {
			};
			sourceItem(decltype(lFile) aFile,
			           decltype(lLineNumber) aLineNumber):
				lFile(aFile), lLineNumber(aLineNumber) {
			};
			decltype(lFile) fGetFile() const {
				return lFile;
			};
			decltype(lLineNumber) fGetLineNumber() const {
				return lLineNumber;
			}
			bool fIsUnset() const {
				return lFile == &sourceFile::gUnsetSource;
			}
		};
	} // end of namespace internal

	std::ostream& operator<< (std::ostream &aStream, const internal::sourceItem& aItem);


	/// wrapper class for fundamental types
	template <typename T> class fundamental_wrapper {
	  protected:
		T lValue;
	  public:
		fundamental_wrapper(T aValue): lValue(aValue) {};
		fundamental_wrapper& operator=(const T aValue) {
			lValue = aValue;
			return *this;
		}
		operator T () const {
			return lValue;
		}
		operator T& () {
			return lValue;
		}
	};


	template <typename T> std::ostream& operator<<(std::ostream& aStream, const fundamental_wrapper<T>& aWrapper) {
		T oerks = aWrapper;
		aStream << oerks;
		return aStream;
	};
	template <typename T> std::istream& operator>>(std::istream& aStream, fundamental_wrapper<T>& aWrapper) {
		T oerks;
		aStream >> oerks;
		aWrapper = oerks;
		return aStream;
	};

	namespace escapedIO {
		std::ostream& operator<<(std::ostream& aStream, const std::string& aString);
		std::istream& operator>>(std::istream& aStream, std::string& aString);
	} // end of namespace escapedIO

/// base class for options

/// Only the templated classes that derive from this base class can contain values.
/// This base class only contains members that are generally usable and do not depend
/// on the type of the option.
	class base {
		friend class parser;
	  protected:
		static std::map<std::string, base*>& fGetOptionMap() {
			static std::map<std::string, base*> gOptions;
			return gOptions;
		};
		static std::map<char, base*>& fGetShortOptionMap() {
			static std::map<char, base*> gShortOptions;
			return gShortOptions;
		};
	  protected:
		char lShortName;
		const std::string lLongName;
		const std::string lExplanation;
		internal::sourceItem lSource;
		short lNargs;
		std::vector<std::string>* lPreserveWorthyStuff;

		std::vector<const base*> lRequiredOptions;
		std::vector<const base*> lForbiddenOptions;

		/// function to set the value from a string, remembering the source
		virtual void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) = 0;
		virtual void fSetMeNoarg(const internal::sourceItem& /*aSource*/) {};
		virtual void fSetSource(const internal::sourceItem& aSource);
	  private:
		virtual void fHandleOption(int argc, const char *argv[], int *i);

	  protected:
		virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const;
		virtual bool fCheckRange(std::ostream& aLogStream) const = 0;
		virtual void fWriteRange(std::ostream &/*aStream*/) const {};
		void fSetPreserveWorthyStuff(std::vector<std::string>* aStuff);

	  public:
		base(char aShortName, const std::string&  aLongName, const std::string&  aExplanation, short aNargs);
		virtual ~base();

		/// special for use in the shellScriptOptionParser
		virtual void fAddToRangeFromStream(std::istream& aStream) = 0; ///< read values from aStream and add them to the range vector
		/// special for use in the shellScriptOptionParser
		virtual void fAddDefaultFromStream(std::istream& aStream) = 0; ///< read a value from aStram and set that as default
		/// write textual representation of value to a std::ostream
		virtual void fWriteValue(std::ostream& aStream) const = 0;
		/// require aOtherOption when this option is set
		virtual void fRequire(const base* aOtherOption);
		/// add vector of other options, particlularly nice for use with initializer list
		virtual void fRequire(std::vector<const base*> aOtherOptions);

		/// add all options from the pair of iterators [aBegin,aEnd) to the list of required options
		template <typename InputIt> void fRequire(InputIt aBegin, InputIt aEnd) {
			for (auto it = aBegin; it != aEnd; ++it) {
				fRequire(*it);
			}
		}
		/// forbid aOtherOption when this option is set
		virtual void fForbid(const base* aOtherOption);
		/// add vector of other options, particlularly nice for use with initializer list
		virtual void fForbid(std::vector<const base*> aOtherOptions);
		/// add all options from the pair of iterators [aBegin,aEnd) to the list of forbidden options
		template <typename InputIt> void fForbid(InputIt aBegin, InputIt aEnd) {
			for (auto it = aBegin; it != aEnd; ++it) {
				fForbid(*it);
			}
		}
		/// check if this option was set, regardless of from command line or config file
		virtual bool fIsSet() const {
			return ! lSource.fIsUnset();
		};

		virtual bool fIsContainer() const {
			return false;
		};


		/// returns long name of option, usually only for internal use.
		const std::string& fGetLongName() const {
			return lLongName;
		};
	};

	namespace internal {
		class positional_base {
		  public:
			positional_base(int aOrderingNumber,
			                base* aAsBase);
			static std::map<int, base*>& fGetPositonalArgs() {
				static std::map<int, base*> gPositinalArgs;
				return gPositinalArgs;
			}
		};
	} // end of namespace internal


/// class that contains the parser, i.e. does that option handling

/// once contructed with the header part and the trailer part of the help text,
/// together with the list of search paths for config files the option parser
/// can then be used to parse the command line options.

	class parser {
	  protected:
		static parser* gParser;
		const char *lDescription;
		const char *lTrailer;
		const std::vector<std::string> lSearchPaths;
		std::vector<std::string> lUnusedOptions;
		std::vector<std::string> lStuffAfterMinusMinus;

		std::set<const base*> lRequiredOptions;

		bool lMinusMinusJustEndsOptions;
		std::ostream *lMessageStream;
		std::ostream *lErrorStream;
		std::string lProgName;
		std::string lExecutableName;
		int lHelpReturnValue;
		char lPrimaryAssignment;
		char lSecondaryAssignment;

		bool lParsingIsDone;

		void fReadConfigFiles();
		void fPrintOptionHelp(std::ostream& aMessageStream, const base& aOption, std::size_t aMaxName, std::size_t aMaxExplain, size_t lineLenght) const;
		void fCheckConsistency();
	  public:
		parser(const char *aDescription = NULL, const char *aTrailer = NULL, const std::vector<std::string>& aSearchPaths = {"/etc/", "~/.", "~/.config/", "./."});
		~parser();

		bool fIsParsingDone() const;

		void fSetMessageStream(std::ostream* aStream);
		void fSetErrorStream(std::ostream* aStream);
		std::ostream& fGetErrorStream() const;
		void fSetHelpReturnValue(int aValue);
		int fGetHelpReturnValue() const {
			return lHelpReturnValue;
		};
		[[noreturn]] virtual void fComplainAndLeave(bool aWithHelp = true);
		void fSetAssignmentChars(char aPrimary = '=', char aSecondary = ':');
		char fGetSecondaryAssignment() const {
			return lSecondaryAssignment;
		}
		virtual void fRequire(const base* aOtherOption);
		virtual void fRequire(std::vector<const base*> aOtherOptions);


		/// parse the options on the command line

		/// The command line is passed via the argc and argv parameres, which will reamain unchanged.
		/// Also reads config files, using fReadConfigFiles() and all files which are explicitly requested by  --readCfgFile options.
		/// \result vector of strings containing all the unhandled string from the original command line
		const std::vector<std::string>& fParse(int argc, const char *argv[]);

		/// other signature for fParse, when argv needs to be without const due to external constraints
		const std::vector<std::string>& fParse(int argc, char *argv[]);


		/// get the only allwed instance of the option parser.
		static parser* fGetInstance();

		/// print help, normally automatically called by the --help option or in case of problems.
		void fHelp();
		void fWriteCfgFile(const char *aFileName);
		void fReadCfgFile(const char *aFileName, const options::internal::sourceItem& aSource, bool aMayBeAbsent = false);
		void fSetExecutableName(const char *aName);

		/// switch on use of -- to separate a trailer on the command line that is not to be parsed
		void fSetMinusMinusStartsExtraList();
		/// get trailong part of command line as a vector od strings, requires that fSetMinusMinusStartsExtraList was called before parsing.
		const std::vector<std::string>& fGetStuffAfterMinusMinus() {
			return lStuffAfterMinusMinus;
		};
		static void fPrintEscapedString(std::ostream &aStream, const char *aString);
		static void fReCaptureEscapedString(char *aDest, const char *aSource);
	};


/// generic option class with any type that can be used with std::istream and std::ostream

/// It is called 'single' because it's meant for single values as opposed to containers
/// as it may contain strings (via specialisations) it's not limited to scalar tyes.
	template <typename T> class single :
		public std::conditional<std::is_fundamental<T>::value,
		fundamental_wrapper<T>,
		T>::type,
		public base {
	  private:
		std::vector<T> lRange;
	  public:
		/// \brief construct an object of single<T>
		/// \copydetails base::base() This generic case always demands one parameter for the option!
		/// \param [in] aDefault default value that the option has if not set otherwise
		/// \param [in] aRange range of allowes values, can be given as initializer list. If only two values are given then [first,last] is the allowd interval.
		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, T aDefault, const std::vector<T>& aRange = {}) :
			std::conditional<std::is_fundamental<T>::value,
			fundamental_wrapper<T>,
			T>::type (aDefault),
			base(aShortName, aLongName, aExplanation, 1) {
			if (!aRange.empty()) {
				fAddToRange(aRange);
			}
		};
		/// add an value to the range of allowed values
		virtual void fAddToRange(T aValue) {
			lRange.push_back(aValue);
		};
		/// add values from the iterator range [aBegin,aEnd) to the range of allowed values
		template <typename InputIt> void fAddToRange(InputIt aBegin, InputIt aEnd) {
			for (auto it = aBegin; it != aEnd; ++it) {
				fAddToRange(*it);
			}
		};
		/// add values from a vector (may be given as initializer list) to the range of allowed values
		virtual void fAddToRange(const std::vector<T>& aRange) {
			fAddToRange(aRange.cbegin(), aRange.cend());
		}
		/// \details read a line from aStream and then add as many values as can be read from that line to the list of allowed values
		virtual void fAddToRangeFromStream(std::istream& aStream) {
			std::string buf;
			std::getline(aStream, buf);
			std::stringstream sbuf(buf);
			while (!sbuf.eof()) {
				T value;
				using escapedIO::operator>>;
				sbuf >> std::setbase(0) >> value;
				fAddToRange(value);
			}
		};
		virtual void fAddDefaultFromStream(std::istream& aStream) {
			using escapedIO::operator>>;
			aStream >> std::setbase(0) >> *this;
		}
		virtual void  fWriteRange(std::ostream &aStream) const {
			using escapedIO::operator<<;
			if (! lRange.empty()) {
				aStream << "# allowed range is";
				if (lRange.size() == 2) {
					aStream << " [" << lRange[0] << "," << lRange[1] << "]\n";
				} else {
					aStream << ":";
					for (auto it = lRange.begin(); it != lRange.end(); ++it) {
						aStream << " " << *it;
					}
					aStream << "\n";
				}
			}
		}

		virtual void fWriteValue(std::ostream& aStream) const {
			using escapedIO::operator<<;
			aStream << *this;
		}
		virtual bool fCheckRange(std::ostream& aLogStream) const {
			using escapedIO::operator<<;
			if (lRange.empty()) {
				return true;
			} else if (lRange.size() == 2) {
				if (lRange[0] <= *this && *this <= lRange[1]) {
					return true;
				} else {
					aLogStream << fGetLongName() << " out of range (" << *this << "), must be in [" << lRange[0] << ", " << lRange[1] << "]\n";
					return false;
				}
			} else {
				for (auto it = lRange.begin(); it != lRange.end(); ++it) {
					if (*it == *this) {
						return true;
					}
				}
				aLogStream << fGetLongName() << " out of range (" << *this << "), must be one of:\n";
				for (auto it = lRange.begin(); it != lRange.end(); ++it) {
					aLogStream << *it << "\n";
				}
				return false;
			}
		}


		virtual void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) {
			using escapedIO::operator>>;
			aStream >> std::setbase(0) >> std::noskipws >> *this;
			if (aStream.fail()) {
				std::string arg;
				aStream >> arg;
				parser::fGetInstance()->fGetErrorStream() << "conversion of '" << arg << "' into " << typeid(T).name() << " failed.\n";
				parser::fGetInstance()->fComplainAndLeave(false);
			}
			fSetSource(aSource);
		}
		const T &fGetValue() const {
			return *this;
		}
	};

/// class specialisation for options of type bool
	template <> class single<bool> : public base {
	  private:
		bool lValue;
		bool lDefault;
	  public:
		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, bool aDefault = false) :
			base(aShortName, aLongName, aExplanation, 0),
			lValue(aDefault), lDefault(aDefault) {
		}
		void fWriteValue(std::ostream& aStream) const override;
		void fSetMeNoarg(const internal::sourceItem& aSource) override;
		void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) override;
		virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
			return true;
		};
		virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
		virtual void fAddDefaultFromStream(std::istream& aStream);

		operator bool () const {
			return lValue;
		}
		bool fGetValue() const {
			return lValue;
		}
	};

/// template specialisation for options that represent simple c-style strings
	template <> class single<const char *> : public base {
	  protected:
		const char *lValue;
		std::vector<const char*> lRange;
	  public:
		single(char aShortName, const std::string& aLongName, const std::string& aExplanation, const char* aDefault = NULL, const std::vector<const char *>& aRange = {});
		virtual ~single();
		virtual void fAddToRange(const char *aValue);
		virtual void fAddToRange(const std::vector<const char *>& aRange);
		/// add values from the iterator range [aBegin,aEnd) to the range of allowed values
		template <typename InputIt> void fAddToRange(InputIt aBegin, InputIt aEnd) {
			for (auto it = aBegin; it != aEnd; ++it) {
				fAddToRange(*it);
			}
		};
		virtual void fAddToRangeFromStream(std::istream& aStream);
		virtual void fAddDefaultFromStream(std::istream& aStream);
		virtual void  fWriteRange(std::ostream &aStream) const;
		virtual void fWriteValue(std::ostream& aStream) const;
		virtual void fSetMe(std::istream& aStream, const internal::sourceItem& aSource);
		virtual bool fCheckRange(std::ostream& aLogStream) const;
		operator const char* () const {
			return lValue;
		}
	};
	/// \namespace options::internal
	/// special namespace for classes and functions that are meant for internal use only

	namespace internal {
/// This class is an intermediate helper class for options that
/// are map-based. It is not to be used directly.
		template <typename T> class baseForMap: public base {
		  protected:
			std::map<const T*, const internal::sourceItem> lSources;
		  public:
			baseForMap(char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs) :
				base(aShortName, aLongName, aExplanation, aNargs) {};
			void fAddSource(const T* aValueLocation, const internal::sourceItem& aSource) {
				lSources.insert(std::make_pair(aValueLocation, aSource));
			};
			const internal::sourceItem fGetSource(const T* aValueLocation) const {
				auto it = lSources.find(aValueLocation);
				if (it != lSources.end()) {
					return it->second;
				} else {
					return internal::sourceItem();
				}
			};
			bool fIsSet() const override {
				return ! lSources.empty();
			};
			bool fIsContainer() const override {
				return true;
			};
		};
	} // end of namespace internal

/// template for map-based options.

/// The map key is always a std::string but the mapped value is arbitrary.
/// the container is by defalt a std::map. It is assumed that the container always containds std::pairs
/// of a std::string as first and the value type T as second, e.g. a
/// std::list<std::pair<std::string,int>> which, in contrast to the map would preserve the order in
/// which the items were specified.
	template <typename T, typename Container = std::map<std::string, T>> class map: public internal::baseForMap<T>, public Container {
	  public:
		map(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
			internal::baseForMap<T>(aShortName, aLongName, aExplanation, 1) {
		}
		virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
		virtual void fAddDefaultFromStream(std::istream& /*aStream*/) {};

		virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const {
			if (this->empty()) {
				aStream << aPrefix << this->lLongName << "=key" << parser::fGetInstance()->fGetSecondaryAssignment() << "value\n";
			}
			for (const auto& it : *this) {
				auto source = this->fGetSource(&(it.second));
				aStream << (source.fIsUnset() ? aPrefix : "") << this->lLongName << "=" << it.first <<  parser::fGetInstance()->fGetSecondaryAssignment();
				{
					using escapedIO::operator<<;
					aStream << it.second << "\n";
				}
				if (!source.fIsUnset()) {
					aStream << "# set from " << source << "\n";
				}
			}
		}

		virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
			return true;
		}

		virtual void fWriteValue(std::ostream& aStream) const {
			using escapedIO::operator<<;
			if (this->empty()) {
				aStream << "\"\"";
			} else {
				for (const auto& it : *this) {
					aStream << it.first << parser::fGetInstance()->fGetSecondaryAssignment() << it.second << " ";
				}
			}
		}
		virtual void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) {
			std::string name;
			std::getline(aStream, name, parser::fGetInstance()->fGetSecondaryAssignment());
			if (aStream.eof()) { // not found, complain!
				parser::fGetInstance()->fGetErrorStream() << "The option " << this->fGetLongName() << " requires a '" << parser::fGetInstance()->fGetSecondaryAssignment() << "' separator, none given\n";
				parser::fGetInstance()->fComplainAndLeave();
			}
			T value;
			using escapedIO::operator>>;
			aStream >> std::setbase(0) >> value;
			if (aStream.fail()) {
				std::string arg;
				aStream >> arg;
				parser::fGetInstance()->fGetErrorStream() << "conversion of '" << arg << "' into " << typeid(value).name() << " failed.\n";
				parser::fGetInstance()->fComplainAndLeave();
			}
			auto result = (*this).insert(this->end(), std::make_pair(name, value));
			this->fAddSource(&(result->second), aSource);
		}
		const std::map<std::string, T>& fGetValue() const  {
			return *static_cast<const std::map<std::string, T>*>(this);
		}
	};

	/// \namespace options::internal
	namespace internal {
/// This class is an intermediate helper class for options that
/// are container-based. It is not to be used directly.
		class baseForContainer: public base {
		  protected:
			std::vector<internal::sourceItem> lSources;
		  public:
			baseForContainer(char aShortName, std::string  aLongName, std::string  aExplanation, short aNargs) :
				base(aShortName, aLongName, aExplanation, aNargs) {};
			bool fIsSet() const override {
				return ! lSources.empty();
			};
			bool fIsContainer() const override {
				return true;
			};
		};
	} // end of namespace internal

/// template for container-based options.

/// the container is by defalt a std::vector.
/// If a non-vector container is used it needs to have a push_back.
	template <typename T, typename Container = std::vector<T>> class container: public internal::baseForContainer, public Container {
	  public:
		container(char aShortName, const std::string& aLongName, const std::string& aExplanation) :
			internal::baseForContainer(aShortName, aLongName, aExplanation, 1) {
		}
		virtual void fAddToRangeFromStream(std::istream& /*aStream*/) {};
		virtual void fAddDefaultFromStream(std::istream& /*aStream*/) {};

		virtual void fWriteCfgLines(std::ostream& aStream, const char *aPrefix) const {
			if (this->empty()) {
				aStream << aPrefix << this->lLongName << "=value\n";
			}
			auto it2 = lSources.begin();
			for (auto it = this->begin(); it != this->end(); ++it, ++it2) {
				aStream << (it2->fIsUnset() ? aPrefix : "") << this->lLongName << "=";
				{
					using escapedIO::operator<<;
					aStream << *it << "\n";
				}
				if (!it2->fIsUnset()) {
					aStream << "# set from " << *it2 << "\n";
				}
			}
		}
		virtual bool fCheckRange(std::ostream& /*aLogStream*/) const {
			return true;
		}


		virtual void fWriteValue(std::ostream& aStream) const {
			if (this->empty()) {
				aStream << "\"\"";
			} else {
				aStream << "\"";
				for (auto it = this->begin(); it != this->end(); ++it) {
					using escapedIO::operator<<;
					aStream << *it << " ";
				}
				aStream << "\"";
			}
		}
		virtual void fSetMe(std::istream& aStream, const internal::sourceItem& aSource) {
			T value;
			aStream >> std::setbase(0);
			{
				using namespace escapedIO;
				aStream >> value;
			}
			if (aStream.fail()) {
				std::string arg;
				aStream >> arg;
				parser::fGetInstance()->fGetErrorStream() << "conversion of '" << arg << "' into " << typeid(value).name() << " failed.\n";
				parser::fGetInstance()->fComplainAndLeave();
			}
			this->push_back(value);
			lSources.push_back(aSource);
		}

		operator const Container& () const {
			return *static_cast<const Container*>(this);
		}
		const Container& fGetValue() const  {
			return *static_cast<const Container*>(this);
		}

	};




	/// interface class that is used for options where the original string rather
	/// than the streamed value is to be written into config files.

	/// This is to be used when the streaming would loose important information,
	/// like having a time point given as 'yesterday' which would loose its relative meaning.
	class originalStringKeeper {
	  protected:
		std::string lOriginalString;
	  public:
		void fWriteOriginalString(std::ostream& aStream) const;
	};



	template <class T> class positional: public T, public internal::positional_base {
	  public:
		template <class ... Types> positional(int aOrderingNumber, Types ... args) :
			T('\0', args...),
			internal::positional_base(aOrderingNumber, this) {
		};
	};

} // end of namespace options
#endif
