// -*-mode: c++; indent-tabs-mode: nil; -*-

#include <iostream>

#include <sys/types.h> // opendir
#include <dirent.h>    // opendir

#include <sys/types.h> // stat
#include <sys/stat.h>  // stat
#include <unistd.h>    // stat

#include <set>
#include <vector>

#include <limits.h>    // realpath
#include <stdlib.h>    // realpath

#include <string.h>    // strerror_r
#include <errno.h>     // errno


#include <limits.h> // PATH_MAX http://stackoverflow.com/a/9449307/257924

namespace VerboseNS {
  
  enum VerbosityLevel {
    E_NONE      = 0
    , E_VERBOSE = 1 << 0
    , E_DEBUG   = 1 << 1
  };

  class VerbosityTracker
  {
  public:

    VerbosityTracker() : _verbosity(E_NONE) {}

    bool isAtVerbosity(int level) { return (_verbosity & level) != 0; }

    void setVerbosity(int value)
    {
      _verbosity = value;
    }

#define DEFINE_SET_VERBOSITY_METHOD(TYPE)       \
  TYPE & setVerbosity(int value) \
  { \
    _verbosity.setVerbosity(value); \
    return *this; \
  }
  
  
  private:
    int _verbosity;
  };

};

class UniqueFileVisitor
{
public:
  virtual bool visit(const std::string & absPath) = 0;
};

class UniqueFileScanner
{
public:
  UniqueFileScanner()
    : _visitor(NULL)
  {}


  UniqueFileScanner & setVisitor(UniqueFileVisitor * visitor)
  {
    _visitor = visitor;
    return *this;
  }

  DEFINE_SET_VERBOSITY_METHOD(UniqueFileScanner);

  bool scan(const std::string & inDir)
  {
    DIR * dirfp = opendir(inDir.c_str());

    if (!dirfp) {
      std::cerr << "ERROR: Failed to open directory: " << inDir << std::endl;
      return false;
    }

    struct dirent entry;
    struct dirent *result = NULL;
    int readdir_retval = 0;
    struct stat statbuf;
    std::string absPath;
    char realpathBuf[PATH_MAX];
    const char * realPath = NULL;
    while ((readdir_retval = readdir_r(dirfp, &entry, &result)) == 0 && result) {

      std::string basename = entry.d_name;

      // Skip directories we obviously should not traverse into:
      if (basename == "." || basename == ".." || basename == ".git")
        continue;

      if (_verbosity.isAtVerbosity(VerboseNS::E_DEBUG)) {
        std::cout << std::endl;
      }

      // Fully-qualify the directory entry in preparation for calling stat:
      absPath = inDir;
      absPath += "/";
      absPath += basename;
      if (_verbosity.isAtVerbosity(VerboseNS::E_DEBUG)) {
        std::cout << "absPath  " << absPath << std::endl;
      }

      // Identify the type of file it is:
      int status = stat(absPath.c_str(), &statbuf);
      if (status == 0) { // stat succeeded

        // Get the realPath for duplicate detection.
        realPath = realpath(absPath.c_str(), realpathBuf);
        // Warn about bad links:
        if (!realPath) {
          char strbuf[1024];
          const char * errstring = strerror_r(errno, strbuf, sizeof(strbuf));
          if (_verbosity.isAtVerbosity(VerboseNS::E_VERBOSE)) {
            std::cerr << "WARNING: Failed resolve link: " << absPath << " : " << errstring << std::endl;
          }
          continue;
        }
        realPath = realPath;
        if (_verbosity.isAtVerbosity(VerboseNS::E_DEBUG)) {
          std::cout << "realPath " << realPath << std::endl;
        }

        // Avoid parsing the same file or directory twice (can happen via symbolic links):
        SetT::iterator iter = _seen.find(realPath);
        if(iter != _seen.end()) {
          if (_verbosity.isAtVerbosity(VerboseNS::E_VERBOSE)) {
            std::cout << "Skipping previously processed file or directory: " << realPath << std::endl;
          }
          continue;
        }
        _seen.insert(realPath);
          
        if ( S_ISDIR(statbuf.st_mode) ) {
          if (_verbosity.isAtVerbosity(VerboseNS::E_DEBUG)) {
            std::cout << "directory absPath " << absPath << std::endl;
          }
          // recurse:
          if (!scan(absPath)) {
            return false;
          }
        } else if ( S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode) ) {
          if (_verbosity.isAtVerbosity(VerboseNS::E_DEBUG)) {
            std::cout << "parsable absPath " << absPath << std::endl;
          }
          if (!_visitor->visit(absPath))
            return false;
        } else {
          if (_verbosity.isAtVerbosity(VerboseNS::E_VERBOSE)) {
            std::cout << "Skipping non-regular file: " << absPath << std::endl;
          }
          continue;
        }
      } else { // stat failed
        if (_verbosity.isAtVerbosity(VerboseNS::E_VERBOSE)) {
          std::cerr << "WARNING: Failed to stat: " << absPath << std::endl;
        }
        continue;
      }

    }

    if (readdir_retval) {
      std::cerr << "ERROR: Failed to read from directory: " << inDir << std::endl;
      return false;
    }

    closedir(dirfp);
    return true;
  }

private:
  typedef std::set<std::string> SetT;
  SetT _seen;
  UniqueFileVisitor * _visitor; // not owned by this object.
  VerboseNS::VerbosityTracker _verbosity;
};

class OrgIDParser : public UniqueFileVisitor
{
public:
 OrgIDParser() {}

  virtual bool visit(const std::string & absPath)
  {

    // Only work on Org-mode files:
    static const char * orgExtension = ".org";
    std::size_t pos;
    if ( (pos = absPath.rfind(orgExtension)) != std::string::npos ) {
      if (_verbosity.isAtVerbosity(VerboseNS::E_VERBOSE)) {
        std::cout << "Processing " << absPath << std::endl;
      }

      // Parse the file:
      return parse(absPath);
      
    }
    return true;
  }

  bool parse(const std::string & absPath)
  {
    mmap(  NULL           // addr: Let the operating system choose
         ,                // length: 
         , PROT_READ      // prot: read-only
         , MAP_PRIVATE    // flags: No need too update the file; we are just reading it.
         ,                // fd:
         ,                // offset:
     )
  }


  DEFINE_SET_VERBOSITY_METHOD(OrgIDParser);

private:
  VerboseNS::VerbosityTracker _verbosity;
};

int main(int argc, char *argv[], char *const envp[])
{
  int verbosity = VerboseNS::E_NONE;
  bool readingDirectories = false;
  typedef std::vector<std::string> DirectoryListT;
  DirectoryListT directories;
  for (int i = 0; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg.empty()) break;
    if (readingDirectories) {
      directories.push_back(arg);
    } else {
      if (arg == "--") {
        readingDirectories = true;
      } else if (arg == "-verbose") {
        verbosity |= VerboseNS::E_VERBOSE;
      } else if (arg == "-debug") {
        verbosity |= VerboseNS::E_DEBUG;
      }
    }
  }

  if (directories.empty()) {
    std::cerr << "ERROR: No directories specified." << std::endl;
    return 1;
  }

  // OrgIDParser will house the results:
  OrgIDParser parser;
  parser.setVerbosity(verbosity);

  // UniqueFileScanner scans for files and removes duplicates, visiting each
  // with the parser:
  UniqueFileScanner scanner;
  scanner
    .setVerbosity(verbosity)
    .setVisitor(&parser);
  for (DirectoryListT::iterator cur = directories.begin(); cur != directories.end(); ++cur)
  {
    int success = scanner.scan(*cur);
    if (!success) {
      return 1;
    }
  }
  // Write out the org id file:
  
  return 0;
} // end main

