// -*-mode: c++; indent-tabs-mode: nil; -*-

#include <iostream>
#include <fstream>

#include <sys/types.h> // opendir
#include <dirent.h>    // opendir

#include <sys/types.h> // stat
#include <sys/stat.h>  // stat
#include <unistd.h>    // stat

#include <limits.h>    // realpath
#include <stdlib.h>    // realpath

#include <string.h>    // strerror_r
#include <errno.h>     // errno

#include <set>
#include <vector>
#include <map>


#include <limits.h> // PATH_MAX http://stackoverflow.com/a/9449307/257924

bool realPathToString(const std::string & inPath, std::string & realPath, bool verbose)
{
  realPath.resize(PATH_MAX);
  // Get the realPath for duplicate detection.
  const char * tmp = realpath(inPath.c_str(), &(realPath[0]));
  // Warn about bad links:
  if (!tmp) {
    char strbuf[1024];
    const char * errstring = strerror_r(errno, strbuf, sizeof(strbuf));
    if (verbose) {
      std::cerr << "WARNING: Failed resolve: " << inPath << " : " << errstring << std::endl;
    }
    realPath = "";
    return false;
  }
  return true;
}

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
    std::string realPath;
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
        if (!realPathToString(absPath, realPath, _verbosity.isAtVerbosity(VerboseNS::E_VERBOSE)))
          continue;

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
    std::ifstream stream(absPath.c_str(), std::ifstream::binary);
    if (!stream) {
      std::cerr << "ERROR: Failed to open: " << absPath << std::endl;
      return false;
    }

    // Get length of file:
    stream.seekg (0, stream.end);
    std::streampos length = stream.tellg();
    stream.seekg (0, stream.beg);

    // Allocate memory:
    std::string buffer(length, 0);

    // Read data as a block:
    stream.read(&(buffer[0]), length);

    stream.close();

    // Print content:
    // std::cout.write(&(buffer[0]), length);
    // std::cout << "<<<" << buffer << ">>>" << std::endl;

     // :PROPERTIES:
     // :ID:       subdir_t-est1-note-2d73-dd0f5b698f14
     // :END:
    std::size_t propBegPos = 0;
    std::size_t propEndPos = std::string::npos;
    std::size_t idLabelBegPos = std::string::npos;
    std::size_t idValueBegPos = std::string::npos;
    std::size_t idValueEndPos = std::string::npos;
    for ( ; (propBegPos = buffer.find(":PROPERTIES:\n", propBegPos)) != std::string::npos; propBegPos++) {
      if ((propEndPos = buffer.find(":END:\n", propBegPos)) == std::string::npos) {
        std::cerr << "ERROR: Lacking :END: starting at position " << propBegPos << " in file " << absPath << std::endl;
        return false;
      }

      static const char * idLabel = ":ID:";
      if ((idLabelBegPos = buffer.find(idLabel, propBegPos)) == std::string::npos || idLabelBegPos > propEndPos) {
        // No id found within this current property list, so move on.
        continue;
      }

      if ((idValueBegPos = buffer.find_first_not_of(" \t", idLabelBegPos + sizeof(idLabel))) == std::string::npos || idValueBegPos > propEndPos) {
        std::cerr << "ERROR: ID lacks a value starting at position " << idLabelBegPos << " in file " << absPath << std::endl;
        return false;
      }

      if ((idValueEndPos = buffer.find_first_of(" \t\n", idValueBegPos)) == std::string::npos || idValueEndPos > propEndPos) {
        std::cerr << "ERROR: ID value premature termination at position " << idValueBegPos << " in file " << absPath << std::endl;
        return false;
      }

      std::string id = buffer.substr(idValueBegPos, idValueEndPos - idValueBegPos);

      if (_verbosity.isAtVerbosity(VerboseNS::E_DEBUG)) {
        std::cout << "absPath " << absPath << " id " << id << std::endl;
      }
      
      IdSetT & ids = _map[absPath]; // Create a IdSetT if one does not exist
      if (ids.find(id) != ids.end()) {
        std::cerr << "ERROR: Duplicate ID value " << id << " at position " << idValueBegPos << " in file " << absPath << std::endl;
        return false;
      }
      ids.insert(id);

    }
      
    return true;
  }

  bool writeIdAlistFile(const std::string & alistPath)
  {
    std::ofstream stream(alistPath.c_str());
    if (!stream) {
      std::cerr << "ERROR: Cannot open " << alistPath << " for writing." << std::endl;
      return false;
    }

    bool first = true;
    stream << "(";
    for (FileToIdSetT::iterator cur = _map.begin(); cur != _map.end(); ++cur)
    {
      const std::string & absPath = cur->first;

      if (!first) {
        stream << std::endl << " ";
      }

      stream << "(\"" << absPath << "\"";
      IdSetT & ids = cur->second;

      for (IdSetT::iterator cur = ids.begin(); cur != ids.end(); ++cur)
      {
        stream << " \"" << *cur << "\"";
      }

      stream << ")";

      first = false;
    }
    stream << ")" << std::endl;

    return true;
  }



  DEFINE_SET_VERBOSITY_METHOD(OrgIDParser);

private:
  VerboseNS::VerbosityTracker _verbosity;
  typedef std::set<std::string> IdSetT;
  typedef std::map<std::string, IdSetT > FileToIdSetT;
  FileToIdSetT _map;
};

int main(int argc, char *argv[], char *const envp[])
{
  int verbosity = VerboseNS::E_NONE;
  bool readingDirectories = false;
  typedef std::vector<std::string> DirectoryListT;
  DirectoryListT directories;
  std::string idAlistPath;
  for (int i = 1; i < argc; i++)
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
      } else if (arg == "-o") {
        if (i + 1 >= argc) {
          std::cerr << "ERROR: -o requires a value." << std::endl;
          return 1;
        }
        idAlistPath = argv[++i];
      } else {
        std::cerr << "ERROR: Unrecognized option " << arg << std::endl;
        return 1;
      }
    }
  }

  if (directories.empty()) {
    std::cerr << "ERROR: No directories specified." << std::endl;
    return 1;
  }

  if (idAlistPath.empty()) {
    std::cerr << "ERROR: No alistfile specified." << std::endl;
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

  // Write out the alist file:
  parser.writeIdAlistFile(idAlistPath);
  
  return 0;
} // end main

