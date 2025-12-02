#include "player.h"
#include <cmath>
#include <chrono>
#include <iostream>

using namespace std::chrono;

Player::Player(unsigned int bpm, unsigned int tpqn)
    : bpm_(bpm), tpqn_(tpqn)
{
  // initialize public output to empty vector
  publicOutputs_.store(std::make_shared<std::vector<float>>());
  running_.store(true);
  thread_ = std::thread(&Player::threadLoop, this);
}

Player::~Player()
{
  running_.store(false);
  cv_.notify_all();
  if (thread_.joinable())
    thread_.join();
}

void Player::play()
{
  playing_.store(true);
  cv_.notify_all();
}

void Player::pause()
{
  playing_.store(false);
}

void Player::reset()
{
  // stop and reset ticks and clip cursors
  playing_.store(false);
  currentTick_.store(0);
  // Reset clip cursors for all clips in all tracks
  for (auto &tr : tracks_)
  {
    for (auto &cl : tr.clips)
    {
      cl.seekToLocalTick(0);
    }
  }
}

void Player::pushCommand(const PlayerCommand &cmd)
{
  {
    std::lock_guard<std::mutex> g(cmdMutex_);
    commandQueue_.push(cmd);
  }
  cv_.notify_all();
}

std::shared_ptr<std::vector<float>> Player::getCurrentTrackOutputs() const
{
  return publicOutputs_.load();
}

void Player::addTrack(const Track &track)
{
  // This is not thread-safe if called concurrently with playback edits.
  // Use pushCommand to add tracks while playing; this helper is convenient
  // before starting playback.
  tracks_.push_back(track);
}

void Player::applyCommands()
{
  std::lock_guard<std::mutex> g(cmdMutex_);
  while (!commandQueue_.empty())
  {
    PlayerCommand cmd = commandQueue_.front();
    commandQueue_.pop();
    if (cmd.type == PlayerCommand::ADD_CLIP_TO_TRACK)
    {
      if (cmd.trackIdx < tracks_.size())
      {
        tracks_[cmd.trackIdx].clips.push_back(std::move(cmd.clip));
      }
      else
      {
        // optionally create a track if out of range
      }
    }
    else if (cmd.type == PlayerCommand::CLEAR_TRACKS)
    {
      tracks_.clear();
    }
    // expand with more command types as required
  }
}

void Player::threadLoop()
{
  auto lastTime = steady_clock::now();
  double secondsPerTick = 60.0 / (static_cast<double>(bpm_.load()) * static_cast<double>(tpqn_.load()));
  unsigned long localTick = currentTick_.load();

  while (running_.load())
  {
    // wait if not playing
    if (!playing_.load())
    {
      std::unique_lock<std::mutex> lk(cvMutex_);
      cv_.wait(lk, [this]
               { return !running_.load() || playing_.load(); });
      lastTime = steady_clock::now();
      continue;
    }

    // apply any pending commands
    applyCommands();

    // compute elapsed time and how many ticks to advance
    auto now = steady_clock::now();
    duration<double> dt = now - lastTime;
    lastTime = now;
    double elapsedSeconds = dt.count() * static_cast<double>(speed_.load());

    // dynamically recompute secondsPerTick in case BPM/TPQN changed via API
    secondsPerTick = 60.0 / (static_cast<double>(bpm_.load()) * static_cast<double>(tpqn_.load()));

    // how many ticks advanced (accumulate fractional ticks by using double)
    static double tickRemainder = 0.0;
    double ticksAdvanced = elapsedSeconds / secondsPerTick + tickRemainder;
    unsigned long wholeTicks = static_cast<unsigned long>(std::floor(ticksAdvanced));
    tickRemainder = ticksAdvanced - static_cast<double>(wholeTicks);

    if (wholeTicks > 0)
    {
      localTick += wholeTicks;
      currentTick_.store(localTick);
    }

    // Evaluate each track and produce a per-track value buffer
    auto outBuf = std::make_shared<std::vector<float>>(tracks_.size(), 0.0f);
    for (size_t ti = 0; ti < tracks_.size(); ++ti)
    {
      float accum = 0.0f;
      Track &tr = tracks_[ti];
      // evaluate each clip in track (each Clip::evaluate is optimized with cursor)
      for (auto &c : tr.clips)
      {
        accum += c.evaluate(localTick);
      }
      // clamp or combine by max if you prefer
      (*outBuf)[ti] = accum;
    }

    // publish buffer to renderer (atomic shared_ptr swap)
    publicOutputs_.store(outBuf);

    // sleep strategy: small sleep to avoid busy loop, will be woken when playing_ changes
    // calculate approximate time to next tick and sleep for a portion of it
    double timeToNextTick = secondsPerTick * (1.0 - tickRemainder);
    // sleep for min( timeToNextTick * 0.5, 5ms )
    double sleepFor = std::min(timeToNextTick * 0.5, 0.005);
    if (sleepFor > 0)
    {
      std::this_thread::sleep_for(duration<double>(sleepFor));
    }
  }
}
