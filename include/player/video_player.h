#ifndef VIDEO_TEXTURE_H
#define VIDEO_TEXTURE_H

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <glad/glad.h>
#include <glm/glm.hpp>

// FFmpeg headers
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
}

class VideoTexture
{
public:
  VideoTexture();
  ~VideoTexture();

  /**
   * Load and initialize video playback
   * @param filename Path to video file
   * @param width Target width for texture
   * @param height Target height for texture
   * @return true if initialization successful
   */
  bool load(const std::string &filename, int width = 1280, int height = 720);

  /**
   * Start video playback
   */
  void play();

  /**
   * Pause video playback
   */
  void pause();

  /**
   * Stop and cleanup video
   */
  void stop();

  /**
   * Update video texture (call every frame)
   * Returns true if a new frame was available
   */
  bool update();

  /**
   * Get the OpenGL texture ID
   */
  GLuint getTextureID() const { return m_textureID; }

  /**
   * Get video dimensions
   */
  glm::ivec2 getDimensions() const { return glm::ivec2(m_width, m_height); }

  /**
   * Check if video is playing
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
   * Set playback speed (1.0 = normal speed)
   */
  void setPlaybackSpeed(float speed) { m_playbackSpeed = speed; }

  /**
   * Loop video when it reaches the end
   */
  void setLooping(bool loop) { m_looping = loop; }

private:
  // OpenGL texture
  GLuint m_textureID{0};
  int m_width{1280};
  int m_height{720};

  // Playback state
  std::atomic<bool> m_isPlaying{false};
  std::atomic<bool> m_shouldStop{false};
  std::atomic<double> m_currentTime{0.0};
  double m_duration{0.0};
  float m_playbackSpeed{1.0f};
  bool m_looping{true};
  double m_lastFrameTime{0.0};

  // FFmpeg context
  AVFormatContext *m_formatCtx{nullptr};
  AVCodecContext *m_codecCtx{nullptr};
  const AVCodec *m_codec{nullptr};
  int m_streamIndex{-1};
  AVFrame *m_frame{nullptr};
  AVFrame *m_frameRGB{nullptr};

  // Color space conversion
  SwsContext *m_swsCtx{nullptr};

  // Thread management
  std::thread m_decoderThread;
  std::mutex m_mutex;
  std::atomic<bool> m_newFrameAvailable{false};

  // Frame storage
  uint8_t *m_frameData{nullptr};
  int m_frameDataSize{0};

  /**
   * Internal frame decoding loop
   */
  void decoderLoop();

  /**
   * Find video stream in format context
   */
  int findVideoStream();

  /**
   * Decode next frame
   */
  bool decodeNextFrame();

  /**
   * Cleanup FFmpeg resources
   */
  void cleanup();

  /**
   * Update OpenGL texture with frame data
   */
  void updateGLTexture(const uint8_t *data);
};

#endif // VIDEO_TEXTURE_H
