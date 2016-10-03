#include <ossia/editor/value/value.hpp>
#include <ossia/network/base/address.hpp>
#include <ossia/network/domain/domain.hpp>
#include <ossia/network/generic/generic_address.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/osc/detail/osc.hpp>
#include <ossia/network/osc/osc.hpp>
#include <ossia/network/exceptions.hpp>
#include <ossia/network/osc/detail/receiver.hpp>
#include <ossia/network/osc/detail/sender.hpp>
#include <oscpack/osc/OscPrintReceivedElements.h>
#include <ossia/detail/logger.hpp>
namespace ossia
{
namespace net
{
osc_protocol::osc_protocol(
    std::string ip, uint16_t remote_port, uint16_t local_port)
    : mIp{ip}
    , mRemotePort{remote_port}
    , mLocalPort{local_port}
    , mSender{std::make_unique<osc::sender>(ip, remote_port)}
    , mReceiver{std::make_unique<osc::receiver>(local_port, [=](const oscpack::ReceivedMessage& m,
                                const oscpack::IpEndpointName& ip) {
                  this->handleReceivedMessage(m, ip);
                })}
{
  if(mReceiver->port() != local_port)
  {;
    throw ossia::connection_error{"osc_protocol::osc_protocol: "
                                  "Could not connect to port: " + std::to_string(local_port)};
  }
  mReceiver->run();
}

osc_protocol::~osc_protocol()
{
}

const std::string& osc_protocol::getIp() const
{
  return mIp;
}

osc_protocol& osc_protocol::setIp(std::string ip)
{
  mIp = ip;
  mSender = std::make_unique<osc::sender>(mIp, mRemotePort);

  return *this;
}

uint16_t osc_protocol::getRemotePort() const
{
  return mRemotePort;
}

osc_protocol& osc_protocol::setRemotePort(uint16_t in_port)
{
  mRemotePort = in_port;
  mSender = std::make_unique<osc::sender>(mIp, mRemotePort);

  return *this;
}

uint16_t osc_protocol::getLocalPort() const
{
  return mLocalPort;
}

osc_protocol& osc_protocol::setLocalPort(uint16_t out_port)
{
  mLocalPort = out_port;
  mReceiver = std::make_unique<osc::receiver>(out_port, [=](const oscpack::ReceivedMessage& m,
                                          const oscpack::IpEndpointName& ip) {
                              this->handleReceivedMessage(m, ip);
                            });
  return *this;
}

bool osc_protocol::getLearningStatus() const
{
  return mLearning;
}

osc_protocol& osc_protocol::setLearningStatus(
    ossia::net::device_base& ossiaDevice, bool newLearn)
{
  return *this;
}

bool osc_protocol::update(ossia::net::node_base& node)
{
  return false;
}

bool osc_protocol::pull(ossia::net::address_base& address)
{
  return false;
}

bool osc_protocol::push(const ossia::net::address_base& address)
{
  auto& addr = static_cast<const generic_address&>(address);

  if (addr.getAccessMode() == ossia::access_mode::GET)
    return false;

  auto val = filter_value(addr);
  if (val.valid())
  {
    mSender->send(address, val);
    if(mLogger.outbound_logger)
        mLogger.outbound_logger->info("Out: {0} {1}", ossia::net::address_string_from_node(address), val);
    return true;
  }
  return false;
}

bool osc_protocol::observe(ossia::net::address_base& address, bool enable)
{
  std::lock_guard<std::mutex> lock(mListeningMutex);

  if (enable)
    mListening.insert(
        std::make_pair(get_osc_address_as_string(address), &address));
  else
    mListening.erase(get_osc_address_as_string(address));

  return true;
}

void osc_protocol::handleReceivedMessage(
    const oscpack::ReceivedMessage& m, const oscpack::IpEndpointName& ip)
{
  std::unique_lock<std::mutex> lock(mListeningMutex);
  auto it = mListening.find(m.AddressPattern());
  if (it != mListening.end())
  {
    ossia::net::address_base& addr = *it->second;
    lock.unlock();
    bool res = update_value(addr, m);

    if(res && mLogger.inbound_logger)
        mLogger.inbound_logger->info("In: {0} {1}", ossia::net::address_string_from_node(addr), addr.cloneValue());
  }
}
}
}
