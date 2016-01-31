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

#ifndef assert
#  define assert(x) if (!(x)) { std::cout << __FILE__ << ":" << __LINE__ << ":" << "ASSERTION FAILED: " #x " is not true" << std::endl; }
#endif

int main(int argc, char *argv[], char *const envp[])
{
  std::cout << __FILE__ << ":" << __LINE__ << ":" << "main begin" << std::endl;
  assert(false);
  std::cout << __FILE__ << ":" << __LINE__ << ":" << "main end" << std::endl;
  return 0;
} // end main

