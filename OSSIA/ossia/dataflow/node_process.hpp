#pragma once
#include <ossia/dataflow/graph_node.hpp>
#include <ossia/editor/scenario/time_interval.hpp>
#include <ossia/editor/scenario/time_process.hpp>

namespace ossia
{

class OSSIA_EXPORT node_process : public ossia::time_process
{
public:
  node_process(ossia::node_ptr n);
  ~node_process() override;
  void offset(ossia::time_value, double pos) override;
  void transport(ossia::time_value date, double pos) override;

  void state(
      ossia::time_value from, ossia::time_value to, double relative_position,
      ossia::time_value tick_offset, double gspeed) override;

  void start() override;
  void stop() override;
  void pause() override;
  void resume() override;
  void mute_impl(bool) override;
};
}
