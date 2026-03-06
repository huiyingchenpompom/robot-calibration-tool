#include "CCSimulatorCameraUdpServer.h"

#include "LogClientCommon.h"

using namespace boost::asio;

CCSimulatorCameraUdpServer::CCSimulatorCameraUdpServer(boost::asio::io_context &context, uint16_t port)
    : m_socket(context)
    , m_endpoint(ip::address::from_string("127.0.0.1"), port)
{
    m_socket.open(m_endpoint.protocol());
    m_socket.bind(m_endpoint);
    memset(m_data, 0, sizeof(m_data));

    this->doReceive();
}

void CCSimulatorCameraUdpServer::setCallFun(std::function<void(const std::string &)> &&fun)
{
    m_callFun = fun;
}

void CCSimulatorCameraUdpServer::doReceive()
{
    m_socket.async_receive_from(
        boost::asio::buffer(m_data, MaxLen), m_endpoint,
        [this](boost::system::error_code ec, std::size_t length)
        {
            if (!ec && length > 0)
            {
                std::string text(m_data, length);
                CCDebug("receive: %s", text.c_str());
                m_callFun(text);
            }
            doReceive();
        });
}
