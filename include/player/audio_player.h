#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

// FFmpeg headers
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
}

class AudioPlayer
{
public:
  AudioPlayer();
  ~AudioPlayer();

  /**
   * Initialize the audio player and start playback thread
   * @param filename Path to audio file to play
   * @return true if initialization successful
   */
  bool init(const std::string &filename);

  /**
   * Start or resume playback
   */
  void play();

  /**
   * Pause playback
   */
  void pause();

  /**
   * Stop playback and cleanup
   */
  void stop();

  /**
   * Check if audio is currently playing
   */
  bool isPlaying() const { return m_isPlaying; }

  /**
   * Get current playback time in seconds
   */
  double getCurrentTime() const { return m_currentTime; }

  /**
   * Get total duration in seconds
   */
  double getDuration() const { return m_duration; }

  /**
   * Set volume (0.0 = mute, 1.0 = normal, > 1.0 = amplified)
   */
  void setVolume(float volume) { m_volume = std::max(0.0f, volume); }

  /**
   * Get current volume
   */
  float getVolume() const { return m_volume; }

private:
  // Playback state
  std::atomic<bool> m_isPlaying{false};
  std::atomic<bool> m_shouldStop{false};
  std::atomic<double> m_currentTime{0.0};
  std::atomic<float> m_volume{0.3f};
  double m_duration{0.0};

  // FFmpeg context
  AVFormatContext *m_formatCtx{nullptr};
  AVCodecContext *m_codecCtx{nullptr};
  const AVCodec *m_codec{nullptr};
  int m_streamIndex{-1};

  // Audio resampling
  SwrContext *m_swrCtx{nullptr};

  // Thread management
  std::thread m_playbackThread;
  std::mutex m_mutex;
  std::condition_variable m_cv;

  // Playback buffer
  std::queue<std::vector<uint8_t>> m_audioBuffer;
  const size_t MAX_BUFFER_SIZE = 10; // Maximum queued buffers

  /**
   * Internal playback loop running in separate thread
   */
  void playbackLoop();

  /**
   * Find audio stream in format context
   */
  int findAudioStream();

  /**
   * Cleanup FFmpeg resources
   */
  void cleanup();

  /**
   * Platform-specific audio output
   */
  void outputAudio(const uint8_t *data, int size);
};

#endif // AUDIO_PLAYER_H
