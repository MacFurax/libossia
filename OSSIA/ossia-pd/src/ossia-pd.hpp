#pragma once
#include <ossia/ossia.hpp>
#include <ossia/network/common/websocket_log_sink.hpp>
#include <sstream>

extern "C" {
#include <cicm_wrapper.h>
}

namespace ossia
{
namespace pd
{

extern "C" void setup_ossia0x2eclient(void);
extern "C" void setup_ossia0x2edevice(void);
extern "C" void setup_ossia0x2emodel(void);
extern "C" void setup_ossia0x2eparam(void);
extern "C" void setup_ossia0x2eremote(void);
extern "C" void setup_ossia0x2eview(void);

class ossia_pd
{
public:
    static ossia_pd& instance();
    static ossia::net::generic_device* get_default_device(){ return &instance().m_device; }

    t_eclass* client{};
    t_eclass* device{};
    t_eclass* logger{};
    t_eclass* model{};
    t_eclass* param{};
    t_eclass* remote{};
    t_eclass* view{};
    t_eclass* ossia{};

private:
    ossia_pd(); // constructor

    ossia::net::local_protocol* m_localProtocol{};
    ossia::net::generic_device m_device;
    string_map<std::shared_ptr<ossia::websocket_threaded_connection>> m_connections;
};

struct t_obj_base;

}
} // namespace
