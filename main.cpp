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
// #include <set>
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
    while ((readdir_retval = readdir_r(dirfp, &entry, &result)) == 0 && result) {
      std::cout << __FILE__ << ":" << __LINE__ << ":" << "entry.d_name " << entry.d_name << std::endl;
    }

    if (readdir_retval) {
      std::cerr << "ERROR: Failed to read from directory: " << inDir << std::endl;
      return false;
    }

    closedir(dirfp);
    return true;
  }
  
}



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

