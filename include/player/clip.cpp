#include "clip.h"

#include <iostream>

Sequence::Sequence() {}
Sequence::~Sequence() {}

Clip::Clip() : t_(0) {}
Clip::~Clip() {}

float Clip::evaluate(unsigned long tick) const
{
  long localSigned = static_cast<long>(tick) - static_cast<long>(t_); // ls - local signed

  if (sequence.keys.empty())
    return 0.0f;

  unsigned int localTick;
  unsigned int dur = sequence.duration();
  if (dur == 0)
  {
    if (sequence.keys.empty())
      return 0.0f;
    return sequence.keys.front().value;
  }

  // Handle before clip starts
  if (localSigned < 0)
    return sequence.keys.front().value;

  // Handle looping or clamping based on loop flag
  if (loop)
  {
    // wrap using modulo to loop infinitely
    unsigned int wrapped = static_cast<unsigned int>(localSigned % static_cast<long>(dur));
    localTick = wrapped;
    // If we wrapped backward relative to cursor (localTick small after large value),
    // reset cursor to avoid long rewind loops
    // Simple heuristic: if localTick < sequence.keys[cursor_].tick, reset cursor
    if (!sequence.keys.empty() && cursor_ < sequence.keys.size() && localTick < sequence.keys[cursor_].tick)
    {
      cursor_ = 0;
    }
  }
  else
  {
    // clamp to [0, dur] - if beyond end, return last value
    if (static_cast<unsigned long>(localSigned) >= static_cast<unsigned long>(dur))
    {
      return sequence.keys.back().value;
    }
    localTick = static_cast<unsigned int>(localSigned);
  }

  // fast path: if only one key, return its value
  if (sequence.keys.size() == 1)
    return sequence.keys.front().value;

  // Ensure cursor_ is within valid range [0, keys.size()-2]
  size_t maxLeft = sequence.keys.size() - 2;
  if (cursor_ > maxLeft)
    cursor_ = maxLeft;

  // Advance cursor while localTick is beyond the right key of current segment
  while (cursor_ < maxLeft && localTick > sequence.keys[cursor_ + 1].tick)
    ++cursor_;

  // Rewind if the localTick moved backwards (seek backwards)
  while (cursor_ > 0 && localTick < sequence.keys[cursor_].tick)
    --cursor_;

  const Keyframe &keyL_ = sequence.keys[cursor_];
  const Keyframe &keyR_ = sequence.keys[cursor_ + 1];

  // // Find keyL_ and keyR_ if they are not set or tick is out of range
  // if (keyL_ == nullptr || keyR_ == nullptr || tick < keyL_->tick || tick > keyR_->tick)
  // {
  //   for (size_t i = 0; i < sequence.keys.size() - 1; i++)
  //   {
  //     if (tick >= sequence.keys[i].tick && tick <= sequence.keys[i + 1].tick)
  //     {
  //       keyL_ = &sequence.keys[i];
  //       keyR_ = &sequence.keys[i + 1];
  //       break;
  //     }
  //   }
  //   // SHOULD NEVER REACH HERE
  //   std::cerr << "Error: Keyframes not found for tick " << tick << "\n";
  //   std::abort();
  // }

  unsigned int span = keyR_.tick - keyL_.tick;
  if (span == 0)
    return keyL_.value; // Avoid division by zero

  float x = (localTick - keyL_.tick) / static_cast<float>(span);
  switch (keyL_.tween)
  {
  case TweenType::STEP:
    return keyL_.value;
  case TweenType::LINEAR:
    return keyL_.value + x * (keyR_.value - keyL_.value);
  case TweenType::EASE_IN:
    x = x * x;
    return keyL_.value + x * (keyR_.value - keyL_.value);
  case TweenType::EASE_OUT:
    x = 1. - (1. - x) * (1. - x);
    return keyL_.value + x * (keyR_.value - keyL_.value);
  case TweenType::EASE_IN_OUT:
    if (x < 0.5f)
    {
      x = 2. * x * x;
      return keyL_.value + 0.5f * x * (keyR_.value - keyL_.value);
    }
    else
    {
      x = -1. + (4. - 2. * x) * x;
      return keyL_.value + 0.5f * x * (keyR_.value - keyL_.value) + 0.5f * (keyR_.value - keyL_.value);
    }
  case TweenType::CUSTOM:
    // Implement custom tweening if needed
    return keyL_.value; // Placeholder
  default:
    return keyL_.value;
  }
}