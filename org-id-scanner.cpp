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

class UniqueFileVisitor
{
public:
  virtual bool visit(const std::string & absPath) = 0;
};

class UniqueFileScanner
{
public:
  UniqueFileScanner()
    : _debug(false)
    , _visitor(NULL)
  {}

  UniqueFileScanner & setDebug(bool debug)
  {
    _debug = debug;
    return *this;
  }

  UniqueFileScanner & setVisitor(UniqueFileVisitor * visitor)
  {
    _visitor = visitor;
    return *this;
  }
  
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

      if (_debug) {
        std::cout << std::endl;
      }

      // Fully-qualify the directory entry in preparation for calling stat:
      absPath = inDir;
      absPath += "/";
      absPath += basename;
      if (_debug) {
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
          std::cerr << "WARNING: Failed resolve link: " << absPath << " : " << errstring << std::endl;
          continue;
        }
        realPath = realPath;
        if (_debug) {
          std::cout << "realPath " << realPath << std::endl;
        }

        // Avoid parsing the same file or directory twice (can happen via symbolic links):
        SetT::iterator iter = _seen.find(realPath);
        if(iter != _seen.end()) {
          std::cout << "Skipping previously processed file or directory: " << realPath << std::endl;
          continue;
        }
        _seen.insert(realPath);
          
        if ( S_ISDIR(statbuf.st_mode) ) {
          if (_debug) {
            std::cout << "directory absPath " << absPath << std::endl;
          }
          // recurse:
          if (!scan(absPath)) {
            return false;
          }
        } else if ( S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode) ) {
          if (_debug) {
            std::cout << "parsable absPath " << absPath << std::endl;
          }
          if (!_visitor->visit(absPath))
            return false;
        } else {
          std::cout << "Skipping non-regular file: " << absPath << std::endl;
          continue;
        }
      } else { // stat failed
        std::cerr << "WARNING: Failed to stat: " << absPath << std::endl;
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
  bool _debug;
  typedef std::set<std::string> SetT;
  SetT _seen;
  UniqueFileVisitor * _visitor; // not owned by this object.
};


class OrgIDParser : public UniqueFileVisitor
{
public:
 OrgIDParser() : _debug(false) {}

  OrgIDParser & setDebug(bool debug)
  {
    _debug = debug;
    return *this;
  }

  virtual bool visit(const std::string & absPath)
  {
    // const char * orgExtension = ".org";
    // std::size_t pos;
    // if ( (pos = absPath.rfind(orgExtension)) != std::string::npos ) {
    //   std::cout << "absPath " << absPath << std::endl;
    // }
    if (_debug) {
      std::cout << "visiting absPath " << absPath << std::endl;
    }

    return true;
  }
  
private:
  bool _debug;
};

int main(int argc, char *argv[], char *const envp[])
{
  std::cout << "main begin" << std::endl;

  bool debug = false;
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
      } else if (arg == "-debug") {
        debug = true;
      }
    }
  }

  if (directories.empty()) {
    std::cerr << "ERROR: No directories specified." << std::endl;
    return 1;
  }

  // OrgIDParser will house the results:
  OrgIDParser parser;
  parser
    .setDebug(debug);

  // UniqueFileScanner
  UniqueFileScanner scanner;
  scanner
    .setDebug(debug)
    .setVisitor(&parser);
  for (DirectoryListT::iterator cur = directories.begin(); cur != directories.end(); ++cur)
  {
    int success = scanner.scan(*cur);
    if (!success) {
      return 1;
    }
  }
  // Write out the org id file:
  

  std::cout << "main end" << std::endl;
  return 0;
} // end main

