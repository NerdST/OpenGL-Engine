#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <string>
#include <queue>
#include <memory>
#include <condition_variable>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

#include "track.h"

// Small command abstraction for thread-safe edits
struct PlayerCommand
{
  // implement basic types; extend as needed
  enum Type
  {
    ADD_CLIP_TO_TRACK,
    CLEAR_TRACKS,
    NOOP,
  } type = NOOP;
  size_t trackIdx = 0;
  Clip clip; // moved/copied into place when needed
};

class Player
{
public:
  Player();
  ~Player();

  /**
   * Constructs a Player with the given BPM and TBP.
   * BPM: Beats Per Minute
   * TBP: Ticks Per Beat
   */
  Player(unsigned int bpm, unsigned int tpqn);

  unsigned long t;

  /**
   * Starts playback of the Player.
   */
  void play();

  /**
   * Pauses playback of the Player.
   */
  void pause();

  /**
   * Resets the Player to the beginning.
   */
  void reset();

  void pushCommand(const PlayerCommand &cmd);

  std::shared_ptr<std::vector<float>> getCurrentTrackOutputs() const;

  void setSpeed(float s) { speed_.store(s); }

  void addTrack(const Track &track);

private:
  void threadLoop();
  void applyCommands();

  std::vector<Track> tracks_;
  mutable std::mutex cmdMutex_;
  std::queue<PlayerCommand> commandQueue_;

  std::thread thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> playing_{false};

  // timing / ticks
  std::atomic<unsigned int> bpm_;  // beats per minute
  std::atomic<unsigned int> tpqn_; // ticks per beat
  std::atomic<float> speed_{1.0f}; // playback speed multiplier

  // current time state (authoritative on player thread)
  std::atomic<unsigned long> currentTick_{0};

  // double-buffered outputs via atmoic shared_ptr swap
  std::atomic<std::shared_ptr<std::vector<float>>> publicOutputs_;

  std::condition_variable cv_;
  mutable std::mutex cvMutex_;
};

#endif // PLAYER_H