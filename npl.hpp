#ifndef NPL_HPP
#define NPL_HPP

#include <CDispatcher.hpp>
#include <CDeviceSocket.hpp>
#include <CProtocolFTP.hpp>
#include <CProtocolWS.hpp>

namespace NPL
{
  auto make_dispatcher(void)
  {
    auto d = std::make_shared<CDispatcher>();
    d->InitializeControl();
    return d;
  }

  auto D = make_dispatcher();

  auto make_ftp(const std::string& host, int port, TLS ftps = TLS::No)
  {
    auto cc = std::make_shared<CDeviceSocket>();
    auto ftp = std::make_shared<CProtocolFTP>();    

    cc->SetHostAndPort(host, port);

    cc->SetTLS(ftps);

    cc->SetProperty("name", "ftp-socket");

    ftp->SetProperty("name", "ftp-protocol");

    D->AddEventListener(cc)->AddEventListener(ftp);

    return ftp;
  }

  auto make_ws_server(const std::string& host, int port, TLS tls = TLS::No, TOnClientMessageCbk cbk = nullptr)
  {
    auto cc = std::make_shared<CDeviceSocket>();
    auto lso = std::make_shared<CProtocolWS>();

    cc->SetTLS(tls);

    cc->SetProperty("name", "ws-cc");

    cc->SetHostAndPort(host, port);

    lso->SetProperty("name", "ws-lso");

    lso->SetClientCallback(cbk);

    D->AddEventListener(cc)->AddEventListener(lso);

    return lso;
  }

  auto make_http_client(const std::string& host, int port)
  {
    auto sock = std::make_shared<CDeviceSocket>();
    auto http = std::make_shared<CProtocolHTTP>();    

    sock->SetHostAndPort(host, port);

    sock->SetProperty("name", "http-socket");

    http->SetProperty("name", "http-protocol");

    D->AddEventListener(sock)->AddEventListener(http);

    return http;
  }

  template <typename T>
  auto make_file(const T& file, bool bCreate = false)
  {
    auto device = std::make_shared<CDevice>(file, bCreate);

    if (device->IsConnected())
    {
      D->AddEventListener(device);
    }
    else
    {
      device.reset();
    }

    return device;
  }

  void TEST_DISPATCHER()
  {
    for (int i = 0; i < 100; i++)
    {
      auto D = std::make_unique<CDispatcher>();
    }

    for (int i = 0; i < 100; i++)
    {
      auto D = std::make_shared<CDispatcher>();
    }

    for (int i = 0; i < 100; i++)
    {
      auto D = new CDispatcher();

      delete D;
    }

    auto D1 = std::make_unique<CDispatcher>();
    auto D2 = std::make_unique<CDispatcher>();
  }
}

#endif