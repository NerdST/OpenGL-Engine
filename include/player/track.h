#ifndef TRACK_H
#define TRACK_H

#include <vector>
#include "clip.h"

class Track
{
public:
  Track();
  ~Track();
  std::vector<Clip> clips;
  std::string name;
};

#endif // TRACK_H