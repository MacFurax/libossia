#pragma once
#if 1 || __has_include(<portaudio.h>)
#include <ossia/audio/audio_protocol.hpp>
#include <portaudio.h>

#if __has_include(<pa_jack.h>) && !defined(_MSC_VER)
#include <pa_jack.h>
#endif
namespace ossia
{
class portaudio_engine final
    : public audio_engine
{
  public:
    int m_ins{}, m_outs{};
    portaudio_engine(std::string name, std::string card_in, std::string card_out,
                     int& inputs, int& outputs, int& rate, int& bs)
    {
      if(Pa_Initialize() != paNoError)
        throw std::runtime_error("Audio error");

#if __has_include(<pa_jack.h>) && !defined(_MSC_VER)
      PaJack_SetClientName(name.c_str());
#endif
      int card_in_idx = -1;
      int card_out_idx = -1;

      for(int i = 0; i < Pa_GetDeviceCount(); i++)
      {
        auto raw_name = Pa_GetDeviceInfo(i)->name;
        if(raw_name == card_in)
        {
          card_in_idx = i;
        }
        if(raw_name == card_out)
        {
          card_out_idx = i;
        }
      }
      if(card_in_idx == -1)
        card_in_idx = Pa_GetDefaultInputDevice();
      if(card_out_idx == -1)
        card_out_idx = Pa_GetDefaultOutputDevice();
      if(card_in_idx == -1 || card_out_idx == -1)
        throw std::runtime_error("Audio error: no default");

      auto devInInfo = Pa_GetDeviceInfo(card_in_idx);
      if(!devInInfo)
        throw std::runtime_error("Audio error: no input device");
      auto devOutInfo = Pa_GetDeviceInfo(card_out_idx);
      if(!devOutInfo)
        throw std::runtime_error("Audio error: no output device");

      inputs = std::min(inputs, devInInfo->maxInputChannels);
      outputs = std::min(outputs, devOutInfo->maxOutputChannels);

      m_ins = inputs;
      m_outs = outputs;

      PaStreamParameters inputParameters;
      inputParameters.device = card_in_idx;
      inputParameters.channelCount = inputs;
      inputParameters.sampleFormat = paFloat32 | paNonInterleaved;
      inputParameters.suggestedLatency = 0.01;
      inputParameters.hostApiSpecificStreamInfo = nullptr;

      PaStreamParameters outputParameters;
      outputParameters.device = card_out_idx;
      outputParameters.channelCount = outputs;
      outputParameters.sampleFormat = paFloat32 | paNonInterleaved;
      outputParameters.suggestedLatency = 0.01;
      outputParameters.hostApiSpecificStreamInfo = nullptr;

      std::cerr << "=== stream start ===\n";
      PaStream* stream;
      auto ec = Pa_OpenStream(&stream,
                              &inputParameters,
                              &outputParameters,
                              rate,
                              bs, //paFramesPerBufferUnspecified,
                              paNoFlag,
                              &PortAudioCallback,
                              this);
      client.store(stream);
      if(ec == PaErrorCode::paNoError)
      {
        ec = Pa_StartStream( client );
        if(ec != PaErrorCode::paNoError)
        {
          std::cerr << "Error while starting audio stream: " << Pa_GetErrorText(ec) << std::endl;
        }
        else
        {
        }
      }
      else
        std::cerr << "Error while opening audio stream: " << Pa_GetErrorText(ec) << std::endl;
    }

    ~portaudio_engine() override
    {
      stop();
      if(protocol)
        protocol.load()->engine = nullptr;

      auto clt = client.load();
      auto ec = Pa_StopStream(clt);
      std::cerr << "=== stream stop ===\n";

      if(ec != PaErrorCode::paNoError)
      {
        std::cerr << "Error while stopping audio stream: " << Pa_GetErrorText(ec) << std::endl;
      }

      Pa_Terminate();
    }

    void reload(ossia::audio_protocol* p) override
    {
      if(this->protocol)
        this->protocol.load()->engine = nullptr;
      stop();

      this->protocol = p;
      if(!p)
        return;
      auto& proto = *p;
      proto.engine = this;

      proto.setup_tree(m_ins, m_outs);

      stop_processing = false;
    }

    void stop() override
    {
      stop_processing = true;
      protocol = nullptr;

      while(processing) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

  private:
    static int PortAudioCallback(
        const void* input,
        void* output,
        unsigned long nframes,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData)
    {
      auto& self = *static_cast<portaudio_engine*>(userData);

      if(self.stop_processing)
      {
        auto float_output = ((float **) output);
        for(std::size_t i = 0; i < self.m_outs; i++)
        {
          auto chan = float_output[i];
          for(std::size_t i = 0; i < nframes; i++)
            chan[i] = 0.;
        }
        return 0;
      }

      auto clt = self.client.load();
      auto proto = self.protocol.load();
      if(clt && proto)
      {
        self.processing = true;
        auto float_input = ((float *const *) input);
        auto float_output = ((float **) output);

        proto->process_generic(*proto, float_input, float_output, (int)self.m_ins, (int)self.m_outs, nframes);
        proto->audio_tick(nframes, timeInfo->currentTime);

        self.processing = false;
      }

      return paContinue;
    }

    std::atomic<PaStream*> client{};
};

}

#endif
