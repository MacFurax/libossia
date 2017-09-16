#pragma once

#include <ossia-pd/src/parameter_base.hpp>
#include <ossia/detail/optional.hpp>
#include <ossia/network/common/path.hpp>

namespace ossia
{
namespace pd
{

class attribute : public parameter_base
{
public:
  using is_view = std::true_type;

  attribute();

  bool register_node(const std::vector<ossia::net::node_base*>& node);
  bool do_registration(const std::vector<ossia::net::node_base*>& node);
  bool unregister();

  ossia::net::device_base* m_dev{};
  t_symbol* m_unit;
  float m_rate_min;

  void set_unit();
  void set_mute();

  void on_parameter_created_callback(const ossia::net::parameter_base& addr);
  static void update_attribute(attribute* x, ossia::string_view attribute);
  static void bind(attribute* x, t_symbol* address);
  static void click( attribute* x, t_floatarg xpos, t_floatarg ypos,
                     t_floatarg shift, t_floatarg ctrl, t_floatarg alt);
  static t_pd_err notify(attribute*x, t_symbol*s, t_symbol* msg, void* sender, void* data);

  static void destroy(attribute* x);
  static void* create(t_symbol* name, int argc, t_atom* argv);

  static ossia::safe_set<attribute*>& quarantine();

  static void get_unit(attribute*x);
  static void get_mute(attribute*x);
  static void get_rate(attribute*x);
  static void get_enable(attribute*x);

  void on_device_deleted(const ossia::net::node_base&);
private:
  void update_path(ossia::string_view name);
  ossia::optional<ossia::traversal::path> m_path;

};
} // namespace pd
} // namespace ossia