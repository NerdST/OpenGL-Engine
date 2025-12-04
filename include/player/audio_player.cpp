#include "audio_player.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#else
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/sample.h>
#endif

AudioPlayer::AudioPlayer()
{
  // Initialize FFmpeg (only needed in older versions, modern FFmpeg doesn't require it)
}

AudioPlayer::~AudioPlayer()
{
  stop();
  cleanup();
}

bool AudioPlayer::init(const std::string &filename)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Open audio file
  int ret = avformat_open_input(&m_formatCtx, filename.c_str(), nullptr, nullptr);
  if (ret < 0)
  {
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Failed to open audio file: " << errbuf << std::endl;
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

  // Find audio stream
  m_streamIndex = findAudioStream();
  if (m_streamIndex < 0)
  {
    std::cerr << "No audio stream found" << std::endl;
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
    return false;
  }

  // Get codec context
  AVStream *stream = m_formatCtx->streams[m_streamIndex];
  m_codec = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!m_codec)
  {
    std::cerr << "Could not find audio codec" << std::endl;
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

  // Initialize resampler to convert to stereo 16-bit PCM at 44.1kHz
  m_swrCtx = swr_alloc();

  // Set channel layout using the new API
  AVChannelLayout in_ch_layout = m_codecCtx->ch_layout;
  AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;

  swr_alloc_set_opts2(&m_swrCtx,
                      &out_ch_layout, AV_SAMPLE_FMT_S16, 44100,
                      &in_ch_layout, (AVSampleFormat)m_codecCtx->sample_fmt, m_codecCtx->sample_rate,
                      0, nullptr);

  ret = swr_init(m_swrCtx);
  if (ret < 0)
  {
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Failed to initialize resampler: " << errbuf << std::endl;
    swr_free(&m_swrCtx);
    avcodec_close(m_codecCtx);
    avcodec_free_context(&m_codecCtx);
    avformat_close_input(&m_formatCtx);
    m_formatCtx = nullptr;
    m_codecCtx = nullptr;
    return false;
  }

  std::cout << "Audio file loaded: " << filename << std::endl;
  std::cout << "  Duration: " << m_duration << " seconds" << std::endl;
  std::cout << "  Sample rate: " << m_codecCtx->sample_rate << " Hz" << std::endl;
  std::cout << "  Channels: " << m_codecCtx->ch_layout.nb_channels << std::endl;

  return true;
}

int AudioPlayer::findAudioStream()
{
  for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++)
  {
    if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      return i;
    }
  }
  return -1;
}

void AudioPlayer::play()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_formatCtx || m_isPlaying)
    return;

  m_shouldStop = false;
  m_isPlaying = true;

  if (m_playbackThread.joinable())
  {
    m_playbackThread.join();
  }

  m_playbackThread = std::thread(&AudioPlayer::playbackLoop, this);
}

void AudioPlayer::pause()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_isPlaying = false;
}

void AudioPlayer::stop()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_shouldStop = true;
    m_isPlaying = false;
  }

  if (m_playbackThread.joinable())
  {
    m_playbackThread.join();
  }

  m_cv.notify_all();
}

void AudioPlayer::playbackLoop()
{
  AVPacket *packet = av_packet_alloc();
  AVFrame *frame = av_frame_alloc();

  if (!packet || !frame)
  {
    std::cerr << "Failed to allocate packet/frame" << std::endl;
    av_packet_free(&packet);
    av_frame_free(&frame);
    return;
  }

  // Allocate output buffer for resampled audio
  uint8_t *out_samples = nullptr;
  int out_linesize = 0;
  int out_samples_per_frame = 0;

  while (!m_shouldStop && m_isPlaying)
  {
    int ret = av_read_frame(m_formatCtx, packet);

    if (ret < 0)
    {
      if (ret == AVERROR_EOF)
      {
        std::cout << "Audio playback finished" << std::endl;
      }
      else
      {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Error reading frame: " << errbuf << std::endl;
      }
      break;
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

    while (avcodec_receive_frame(m_codecCtx, frame) == 0)
    {
      // Allocate output buffer if needed
      if (!out_samples || out_samples_per_frame != frame->nb_samples)
      {
        out_samples_per_frame = frame->nb_samples;
        av_freep(&out_samples);
        av_samples_alloc(&out_samples, &out_linesize, 2, out_samples_per_frame, AV_SAMPLE_FMT_S16, 0);
      }

      // Resample audio
      int nb_samples = swr_convert(m_swrCtx, &out_samples, out_samples_per_frame,
                                   (const uint8_t **)frame->data, frame->nb_samples);

      if (nb_samples > 0)
      {
        int out_size = av_samples_get_buffer_size(&out_linesize, 2, nb_samples, AV_SAMPLE_FMT_S16, 1);

        // Apply volume control
        float volume = m_volume.load();
        if (volume != 1.0f)
        {
          int16_t *samples = (int16_t *)out_samples;
          int num_samples = out_size / sizeof(int16_t);
          for (int i = 0; i < num_samples; i++)
          {
            float sample = samples[i] * volume;
            // Clamp to prevent overflow
            if (sample > 32767.0f)
              samples[i] = 32767;
            else if (sample < -32768.0f)
              samples[i] = -32768;
            else
              samples[i] = (int16_t)sample;
          }
        }

        // Update current time
        if (frame->pts != AV_NOPTS_VALUE)
        {
          AVStream *stream = m_formatCtx->streams[m_streamIndex];
          m_currentTime = static_cast<double>(frame->pts) * av_q2d(stream->time_base);
        }

        // Output audio (this will block appropriately for real-time playback)
        outputAudio(out_samples, out_size);
      }
    }
  }

  // Cleanup
  av_freep(&out_samples);
  av_frame_free(&frame);
  av_packet_free(&packet);

  m_isPlaying = false;
}

void AudioPlayer::outputAudio(const uint8_t *data, int size)
{
#ifdef _WIN32
  // Windows implementation using waveOut
  static HWAVEOUT hWaveOut = nullptr;
  static bool initialized = false;

  if (!initialized)
  {
    WAVEFORMATEX wfx;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nChannels = 2;
    wfx.cbSize = 0;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    initialized = true;
  }

  // Note: This is a simplified implementation. In production, you'd want to use
  // waveOutWrite with proper buffer management
  Sleep(size / (44100.0 * 2 * 2) * 1000.0); // Sleep for the duration of audio
#else
  // Linux implementation using PulseAudio
  static pa_simple *s = nullptr;
  static bool initialized = false;

  if (!initialized)
  {
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 2;
    ss.rate = 44100;

    int error = 0;
    s = pa_simple_new(nullptr, nullptr, PA_STREAM_PLAYBACK, nullptr, "LearnOpenGL", &ss, nullptr, nullptr, &error);
    if (!s)
    {
      std::cerr << "PulseAudio connection failed: " << pa_strerror(error) << std::endl;
      return;
    }
    initialized = true;
  }

  if (s)
  {
    int error;
    if (pa_simple_write(s, data, size, &error) < 0)
    {
      std::cerr << "pa_simple_write() failed: " << pa_strerror(error) << std::endl;
    }
  }
#endif
}

void AudioPlayer::cleanup()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_swrCtx)
  {
    swr_free(&m_swrCtx);
    m_swrCtx = nullptr;
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
}
