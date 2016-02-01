// -*-mode: c++; indent-tabs-mode: nil; -*-

#include <iostream>
// #include <deque>
// #include <float.h>           // Defines FLT_MAX
// #include <fstream>
// #include <ios>
// #include <limits>
// #include <limits.h>          // Defines LONG_MAX/INT_MAX
// #include <map>
// #include <math.h>            // Defines fabs
// #include <sstream>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/stat.h>
// #include <sys/time.h> // gettimeofday
// #include <sys/types.h>
// #include <unistd.h>
// #include <vector>

#include <sys/types.h> // opendir
#include <dirent.h>    // opendir

#include <sys/types.h> // stat
#include <sys/stat.h>  // stat
#include <unistd.h>    // stat

#include <set>

#include <limits.h>    // realpath
#include <stdlib.h>    // realpath

#include <string.h>    // strerror_r
#include <errno.h>     // errno


//#include <linux/limits.h> // PATH_MAX http://stackoverflow.com/a/9449307/257924
#include <limits.h> // PATH_MAX http://stackoverflow.com/a/9449307/257924

#ifndef assert
#  define assert(x) if (!(x)) { std::cout << __FILE__ << ":" << __LINE__ << ":" << "ASSERTION FAILED: " #x " is not true" << std::endl; }
#endif

class OrgIdScanner {
public:
  OrgIdScanner() {}
  
  bool parseDirectory(const std::string & inDir)
  {
    DIR * dirfp = opendir(inDir.c_str());

    if (!dirfp) {
      std::cerr << "ERROR: Failed to open directory: " << inDir << std::endl;
      return false;
    }

    struct dirent entry;
    struct dirent *result = NULL;
    int readdir_retval = 0;
    // const char * orgExtension = ".org";
    // std::size_t pos;
    struct stat statbuf;
    std::string absPath;
    char realpathBuf[PATH_MAX];
    const char * realPathTmp = NULL;
    while ((readdir_retval = readdir_r(dirfp, &entry, &result)) == 0 && result) {

      std::string basename = entry.d_name;

      // Skip directories we obviously should not traverse into:
      if (basename == "." || basename == ".." || basename == ".git")
        continue;

      std::cout << std::endl;

      // Fully-qualify the directory entry in preparation for calling stat:
      absPath = inDir;
      absPath += "/";
      absPath += basename;
      std::cout << __FILE__ << ":" << __LINE__ << ":" << "absPath  " << absPath << std::endl;

      // Identify the type of file it is:
      int status = stat(absPath.c_str(), &statbuf);
      if (status == 0) { // stat succeeded

        const char * realPath = NULL;

        // Get the realPath:
        if ( S_ISLNK(statbuf.st_mode) ) {
          std::cout << __FILE__ << ":" << __LINE__ << ":" << "found link" << std::endl;
          realPathTmp = realpath(absPath.c_str(), realpathBuf);
          // Warn about bad links:
          if (!realPathTmp) {
            char strbuf[1024];
            const char * errstring = strerror_r(errno, strbuf, sizeof(strbuf));
            std::cerr << "WARNING: Failed resolve link: " << absPath << " : " << errstring << std::endl;
            continue;
          }
          realPath = realPathTmp;
        } else {
          std::cout << __FILE__ << ":" << __LINE__ << ":" << "found non-link" << std::endl;
          realPath = absPath.c_str();
        }

        std::cout << __FILE__ << ":" << __LINE__ << ":" << "realPath " << realPath << std::endl;

        // Avoid parsing the same file/directory twice (can happen via symbolic links):
        SetT::iterator iter = _seen.find(realPath);
        if(iter != _seen.end()) {
          std::cout << "Skipping previously processed file/directory: " << realPath << std::endl;
          continue;
        }
        _seen.insert(realPath);
          
        if ( S_ISDIR(statbuf.st_mode) ) {
          std::cout << __FILE__ << ":" << __LINE__ << ":" << "directory absPath " << absPath << std::endl;
          // recurse:
          if (!parseDirectory(absPath)) {
            return false;
          }
        } else if ( S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode) ) {
          std::cout << __FILE__ << ":" << __LINE__ << ":" << "parsable absPath " << absPath << std::endl;
        } else {
          std::cout << "Skipping non-regular file: " << absPath << std::endl;
        }
      } else { // stat failed
        std::cerr << "WARNING: Failed to stat: " << absPath << std::endl;
        continue;
      }

      // if ( (pos = absPath.rfind(orgExtension)) != std::string::npos ) {
      //   std::cout << __FILE__ << ":" << __LINE__ << ":" << "absPath " << absPath << std::endl;
      // }
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
};



int main(int argc, char *argv[], char *const envp[])
{
  std::cout << __FILE__ << ":" << __LINE__ << ":" << "main begin" << std::endl;

  std::string inDir;
  for (int i = 0; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg.empty()) break;
    if (arg == "-d") {
      if (i + 1 >= argc) {
        std::cerr << "ERROR: Missing value of -d option." << std::endl;
        return 1;
      }
      inDir = argv[++i];
    }
  }

  if (inDir.empty()) {
    std::cerr << "ERROR: -d option was not specified." << std::endl;
    return 1;
  }

  //std::cout << __FILE__ << ":" << __LINE__ << ":" << "inDir " << inDir << std::endl;

  OrgIdScanner scanner;
  int success = scanner.parseDirectory(inDir);
  if (!success) {
    return 1;
  }

  // Write out the org id file:
  

  std::cout << __FILE__ << ":" << __LINE__ << ":" << "main end" << std::endl;
  return 0;
} // end main

