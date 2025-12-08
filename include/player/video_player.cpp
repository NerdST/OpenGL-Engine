#include "video_player.h"
#include <iostream>
#include <chrono>
#include <cstring>

extern "C"
{
#include <libavutil/imgutils.h>
}

VideoTexture::VideoTexture()
{
}

VideoTexture::~VideoTexture()
{
  stop();
  cleanup();
}

bool VideoTexture::load(const std::string &filename, int width, int height)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Open video file FIRST to get actual dimensions
  int ret = avformat_open_input(&m_formatCtx, filename.c_str(), nullptr, nullptr);
  if (ret < 0)
  {
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Failed to open video file: " << errbuf << std::endl;
    return false;
  }

  // Find stream info
  ret = avformat_find_stream_info(m_formatCtx, nullptr);
  if (ret < 0)
  {
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Failed to find stream info: " << errbuf << std::endl;
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
    return false;
  }

  // Find video stream
  m_streamIndex = findVideoStream();
  if (m_streamIndex < 0)
  {
    std::cerr << "No video stream found" << std::endl;
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
    return false;
  }

  // Use the video's actual dimensions, not the requested ones
  AVStream *stream = m_formatCtx->streams[m_streamIndex];
  m_width = stream->codecpar->width;
  m_height = stream->codecpar->height;

  std::cout << "Using video native resolution: " << m_width << "x" << m_height << std::endl;

  // Create OpenGL texture with actual video dimensions
  // Use GL_SRGB8 for proper color space - video files are typically in sRGB
  glGenTextures(1, &m_textureID);
  glBindTexture(GL_TEXTURE_2D, m_textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Allocate frame data buffer
  m_frameDataSize = m_width * m_height * 3; // RGB
  m_frameData = new uint8_t[m_frameDataSize];
  std::memset(m_frameData, 0, m_frameDataSize);

  // Get codec context
  m_codec = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!m_codec)
  {
    std::cerr << "Could not find video codec" << std::endl;
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
    return false;
  }

  m_codecCtx = avcodec_alloc_context3(m_codec);
  avcodec_parameters_to_context(m_codecCtx, stream->codecpar);

  // Open codec
  ret = avcodec_open2(m_codecCtx, m_codec, nullptr);
  if (ret < 0)
  {
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Failed to open codec: " << errbuf << std::endl;
    avcodec_free_context(&m_codecCtx);
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
    m_codecCtx = nullptr;
    return false;
  }

  // Calculate duration
  if (m_formatCtx->duration != AV_NOPTS_VALUE)
  {
    m_duration = static_cast<double>(m_formatCtx->duration) / AV_TIME_BASE;
  }

  // Allocate frame structures
  m_frame = av_frame_alloc();
  m_frameRGB = av_frame_alloc();

  if (!m_frame || !m_frameRGB)
  {
    std::cerr << "Failed to allocate frames" << std::endl;
    av_frame_free(&m_frame);
    av_frame_free(&m_frameRGB);
    avcodec_close(m_codecCtx);
    avcodec_free_context(&m_codecCtx);
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
    m_codecCtx = nullptr;
    m_frame = nullptr;
    m_frameRGB = nullptr;
    return false;
  }

  AVColorRange range = m_codecCtx->color_range;
  std::cout << "Video color range: " << range << std::endl;

  // Setup color space conversion
  m_swsCtx = sws_getContext(
      m_codecCtx->width, m_codecCtx->height, m_codecCtx->pix_fmt,
      m_width, m_height, AV_PIX_FMT_RGB24,
      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  if (!m_swsCtx)
  {
    std::cerr << "Failed to create color conversion context" << std::endl;
    av_frame_free(&m_frame);
    av_frame_free(&m_frameRGB);
    avcodec_close(m_codecCtx);
    avcodec_free_context(&m_codecCtx);
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
    m_codecCtx = nullptr;
    m_frame = nullptr;
    m_frameRGB = nullptr;
    return false;
  }

  // Prepare frame for RGB data
  av_image_fill_arrays((uint8_t **)m_frameRGB->data, m_frameRGB->linesize, m_frameData,
                       AV_PIX_FMT_RGB24, m_width, m_height, 1);
  std::cout << "Video loaded: " << filename << std::endl;
  std::cout << "  Resolution: " << m_codecCtx->width << "x" << m_codecCtx->height << std::endl;
  std::cout << "  Duration: " << m_duration << " seconds" << std::endl;
  std::cout << "  FPS: " << av_q2d(stream->r_frame_rate) << std::endl;

  return true;
}

int VideoTexture::findVideoStream()
{
  for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++)
  {
    if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      return i;
    }
  }
  return -1;
}

void VideoTexture::play()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (!m_formatCtx || m_isPlaying)
    return;

  m_shouldStop = false;
  m_isPlaying = true;
  m_lastFrameTime = 0.0;

  if (m_decoderThread.joinable())
  {
    m_decoderThread.join();
  }

  m_decoderThread = std::thread(&VideoTexture::decoderLoop, this);
}

void VideoTexture::pause()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_isPlaying = false;
}

void VideoTexture::stop()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_shouldStop = true;
    m_isPlaying = false;
  }

  if (m_decoderThread.joinable())
  {
    m_decoderThread.join();
  }
}

bool VideoTexture::update()
{
  if (!m_newFrameAvailable || !m_frameData)
    return false;

  m_newFrameAvailable = false;

  std::lock_guard<std::mutex> lock(m_mutex);

  // static int update_count = 0;
  // if (++update_count % 10 == 0)
  // {
  //   std::cout << "Updating GL texture, frame data[0]: " << (int)m_frameData[0]
  //             << " (resolution: " << m_width << "x" << m_height << ")" << std::endl;
  // }
  updateGLTexture(m_frameData);
  return true;
}

void VideoTexture::decoderLoop()
{
  AVPacket *packet = av_packet_alloc();
  if (!packet)
  {
    std::cerr << "Failed to allocate packet" << std::endl;
    return;
  }

  auto stream = m_formatCtx->streams[m_streamIndex];
  double fps = av_q2d(stream->r_frame_rate);
  double frame_delay = 1.0 / fps;

  auto start_time = std::chrono::high_resolution_clock::now();

  while (!m_shouldStop && m_isPlaying)
  {
    int ret = av_read_frame(m_formatCtx, packet);

    if (ret < 0)
    {
      if (ret == AVERROR_EOF)
      {
        std::cout << "Video playback finished" << std::endl;

        if (m_looping)
        {
          // Seek to beginning
          av_seek_frame(m_formatCtx, m_streamIndex, 0, AVSEEK_FLAG_BACKWARD);
          avcodec_flush_buffers(m_codecCtx);
          start_time = std::chrono::high_resolution_clock::now();
          m_currentTime = 0.0;
        }
        else
        {
          m_isPlaying = false;
          break;
        }
      }
      else
      {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Error reading frame: " << errbuf << std::endl;
        break;
      }
      continue;
    }

    if (packet->stream_index != m_streamIndex)
    {
      av_packet_unref(packet);
      continue;
    }

    ret = avcodec_send_packet(m_codecCtx, packet);
    av_packet_unref(packet);

    if (ret < 0)
    {
      char errbuf[128];
      av_strerror(ret, errbuf, sizeof(errbuf));
      std::cerr << "Error sending packet: " << errbuf << std::endl;
      continue;
    }

    if (avcodec_receive_frame(m_codecCtx, m_frame) == 0)
    {
      // Convert frame to RGB
      sws_scale(m_swsCtx, (const uint8_t *const *)m_frame->data, m_frame->linesize,
                0, m_codecCtx->height, m_frameRGB->data, m_frameRGB->linesize);

      // Update timing
      if (m_frame->pts != AV_NOPTS_VALUE)
      {
        m_currentTime = static_cast<double>(m_frame->pts) * av_q2d(stream->time_base);
      }

      m_newFrameAvailable = true;
      // static int frame_count = 0;
      // if (++frame_count % 30 == 0)
      // {
      //   std::cout << "Decoded frame " << frame_count << ", time: " << m_currentTime << "s" << std::endl;
      // }

      // Frame timing synchronization
      auto now = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(now - start_time).count();
      double target_time = m_currentTime / m_playbackSpeed;

      if (elapsed < target_time)
      {
        double sleep_time = (target_time - elapsed) * 1000.0; // Convert to ms
        std::this_thread::sleep_for(std::chrono::milliseconds((long long)sleep_time));
      }
    }
  }

  av_packet_free(&packet);
}

void VideoTexture::updateGLTexture(const uint8_t *data)
{
  if (m_textureID == 0)
    return;

  if (!data || !m_frameData)
  {
    std::cerr << "updateGLTexture: No data available" << std::endl;
    return;
  }

  glBindTexture(GL_TEXTURE_2D, m_textureID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, data);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoTexture::cleanup()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_textureID != 0)
  {
    glDeleteTextures(1, &m_textureID);
    m_textureID = 0;
  }

  if (m_swsCtx)
  {
    sws_freeContext(m_swsCtx);
    m_swsCtx = nullptr;
  }

  if (m_frameRGB)
  {
    av_frame_free(&m_frameRGB);
    m_frameRGB = nullptr;
  }

  if (m_frame)
  {
    av_frame_free(&m_frame);
    m_frame = nullptr;
  }

  if (m_codecCtx)
  {
    avcodec_close(m_codecCtx);
    avcodec_free_context(&m_codecCtx);
    m_codecCtx = nullptr;
  }

  if (m_formatCtx)
  {
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
  }

  if (m_frameData)
  {
    delete[] m_frameData;
    m_frameData = nullptr;
  }
}
