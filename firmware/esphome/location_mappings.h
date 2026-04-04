#include <algorithm>
#include <cctype>

using namespace std;

int location_int(string loc) {
  transform(loc.begin(), loc.end(), loc.begin(), tolower);
  switch (loc) {
  case "home":
    return 0;
  case "work":
  case "school":
    return 1;
  case "bus":
    return 2;
  case "car":
    return 3;
  case "shop":
    return 4;
  default: return 3; // not home
  }
};
