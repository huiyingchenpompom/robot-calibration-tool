#ifndef CCSIMULATORCAMERAUDPSERVER_H
#define CCSIMULATORCAMERAUDPSERVER_H

#include <boost/asio.hpp>

using boost::asio::ip::udp;

class CCSimulatorCameraUdpServer
{
public:
    CCSimulatorCameraUdpServer(boost::asio::io_context &context, uint16_t port);
    void setCallFun(std::function<void(const std::string &)> &&fun);

protected:
    void doReceive();

private:
    udp::socket m_socket;
    udp::endpoint m_endpoint;
    std::function<void(const std::string &)> m_callFun;

    enum { MaxLen = 1024 };
    char m_data[MaxLen];
};

#endif // CCSIMULATORCAMERAUDPSERVER_H
