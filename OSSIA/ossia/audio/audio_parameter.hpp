#pragma once

#include <ossia/dataflow/data.hpp>
#include <ossia/dataflow/graph_node.hpp>
#include <ossia/network/base/parameter.hpp>
#if defined(__EMSCRIPTEN__)
#define GSL_USE_STD_BYTE 0
#endif
#include <gsl/span>

namespace ossia
{
class OSSIA_EXPORT audio_parameter : public ossia::net::parameter_base
{

public:
  ossia::small_vector<gsl::span<float>, 8> audio;

  audio_parameter(ossia::net::node_base& n);

  virtual ~audio_parameter();


  void clone_value(audio_vector& res) const;
  void push_value(const audio_port& port);

  void pull_value() override;
  net::parameter_base& push_value(const ossia::value&) override;
  net::parameter_base& push_value(ossia::value&&) override;
  net::parameter_base& push_value() override;
  ossia::value value() const override;
  net::parameter_base& set_value(const ossia::value&) override;
  net::parameter_base& set_value(ossia::value&&) override;
  val_type get_value_type() const override;
  net::parameter_base& set_value_type(val_type) override;
  access_mode get_access() const override;
  net::parameter_base& set_access(access_mode) override;
  const domain& get_domain() const override;
  net::parameter_base& set_domain(const domain&) override;
  bounding_mode get_bounding() const override;
  net::parameter_base& set_bounding(bounding_mode) override;
};

class OSSIA_EXPORT virtual_audio_parameter final : public audio_parameter
{
  // todo use a flat vector instead for perf
  std::vector<std::vector<float>> m_audio_data;

public:
  virtual_audio_parameter(int num_channels, ossia::net::node_base& n)
    : audio_parameter{n}
    , m_audio_data(num_channels)
  {
    set_buffer_size(512);
  }

  void set_buffer_size(int bs)
  {
    const auto chan = m_audio_data.size();
    audio.resize(chan);
    for(std::size_t i = 0; i < chan; i++)
    {
      m_audio_data[i].resize(bs);
      audio[i] = m_audio_data[i];
      ossia::fill(m_audio_data[i], 0.f);
    }
  }

  virtual ~virtual_audio_parameter();
};

using audio_mapping = std::vector<int>;
class OSSIA_EXPORT mapped_audio_parameter final : public audio_parameter
{
public:
  audio_mapping mapping;
  bool is_output{false};
  mapped_audio_parameter(bool output, audio_mapping m, ossia::net::node_base& n);

  virtual ~mapped_audio_parameter();
};

class OSSIA_EXPORT texture_generic_parameter : public ossia::net::parameter_base
{
  int32_t m_tex{};

public:
  texture_generic_parameter(ossia::net::node_base& n)
    : ossia::net::parameter_base{n}
  {
  }

  virtual ~texture_generic_parameter();

  void clone_value() const
  {
  }

  void push_value(int)
  {
  }

  void pull_value() override
  {
  }
  net::parameter_base& push_value(const ossia::value&) override
  {
    return *this;
  }
  net::parameter_base& push_value(ossia::value&&) override
  {
    return *this;
  }
  net::parameter_base& push_value() override
  {
    return *this;
  }
  ossia::value value() const override
  {
    return {};
  }
  net::parameter_base& set_value(const ossia::value&) override
  {
    return *this;
  }
  net::parameter_base& set_value(ossia::value&&) override
  {
    return *this;
  }
  val_type get_value_type() const override
  {
    return {};
  }
  net::parameter_base& set_value_type(val_type) override
  {
    return *this;
  }
  access_mode get_access() const override
  {
    return {};
  }
  net::parameter_base& set_access(access_mode) override
  {
    return *this;
  }
  const domain& get_domain() const override
  {
    throw;
  }
  net::parameter_base& set_domain(const domain&) override
  {
    return *this;
  }
  bounding_mode get_bounding() const override
  {
    return {};
  }
  net::parameter_base& set_bounding(bounding_mode) override
  {
    return *this;
  }
};


struct execution_state;
class OSSIA_EXPORT sound_node final :
    public ossia::nonowning_graph_node
{
public:
  sound_node();
  ~sound_node();

  void set_start(std::size_t v) { start = v; }
  void set_upmix(std::size_t v) { upmix = v; }
  void set_sound(const std::vector<std::vector<float>>& vec);
  void set_sound(std::vector<std::vector<double>> vec);
  void run(ossia::token_request t, ossia::execution_state& e) override;
  std::size_t channels() const { return m_data.size(); }
  std::size_t duration() const { return m_data.empty() ? 0 : m_data[0].size(); }

private:
  std::vector<std::vector<double>> m_data;
  std::size_t start{};
  std::size_t upmix{};
  ossia::outlet audio_out{ossia::audio_port{}};
};

OSSIA_EXPORT
void do_fade(bool start_discontinuous, bool end_discontinuous, audio_channel& ap, std::size_t start, std::size_t end);
}
