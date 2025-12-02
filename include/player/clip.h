#ifndef ENVELOPECURVE_H
#define ENVELOPECURVE_H

#include <vector>
#include <string>

enum class TweenType
{
  STEP,
  LINEAR,
  EASE_IN,
  EASE_OUT,
  EASE_IN_OUT,
  CUSTOM
};

struct Keyframe
{
  unsigned int tick; // in local clip tick space NOT GLOBAL TICKS
  float value;
  TweenType tween = TweenType::LINEAR;

  Keyframe(unsigned int t, float v, TweenType tt = TweenType::LINEAR)
      : tick(t), value(v), tween(tt) {}
};

class Sequence
{
public:
  Sequence();
  ~Sequence();
  std::vector<Keyframe> keys;
  std::string name;

  unsigned int duration() const
  {
    if (keys.empty())
      return 0;
    return keys.back().tick;
  }

private:
  // unsigned long t_; // sequence location in ticks
  // Store pointer values
};

class Clip
{
public:
  Clip();
  ~Clip();

  /**
   * Constructs a Clip at global tick t with the given Sequence.
   */
  Clip(Sequence seq, unsigned long t) : sequence(seq), t_(t) {}

  std::string name;
  Sequence sequence;
  bool loop = false;

  /**
   * Evaluates the sequence at the given tick in global tick space.
   */
  float evaluate(unsigned long tick) const;

  void seekToLocalTick(unsigned long localTick) const
  {
    // Reset cursor based on localTick
    cursor_ = 0;
    if (sequence.keys.size() < 2)
      return;
    while (cursor_ + 1 < sequence.keys.size() - 1 && localTick > sequence.keys[cursor_ + 1].tick)
      ++cursor_;
  }

private:
  unsigned long t_; // global clip location in ticks
  // mutable const Keyframe *keyL_ = nullptr, *keyR_ = nullptr;
  mutable size_t cursor_ = 0;
};

#endif // ENVELOPECURVE_H