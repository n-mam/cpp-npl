#include <CDispatcher.hpp>
#include <CDeviceSocket.hpp>
#include <CProtocolFTP.hpp>
#include <CProtocolWS.hpp>
#include <Json.hpp>

NS_NPL

auto make_dispatcher(void)
{
  auto d = std::make_shared<CDispatcher>();

  d->InitializeControl();

  return d;
}

auto D = make_dispatcher();

auto make_ftp(const std::string& host, int port, TLS ftps = TLS::NO)
{
  auto cc = std::make_shared<CDeviceSocket>();
  auto ftp = std::make_shared<CProtocolFTP>();    

  cc->SetHostAndPort(host, port);

  cc->SetTLS(ftps);

  cc->SetName("ftp-cc");

  ftp->SetName("ftp");

  D->AddEventListener(cc)->AddEventListener(ftp);

  return ftp;
}

auto make_ws_server(const std::string& host, int port, TLS tls = TLS::NO, TOnClientMessageCbk cbk = nullptr)
{
  auto cc = std::make_shared<CDeviceSocket>();
  auto lso = std::make_shared<CProtocolWS>();

  cc->SetTLS(tls);

  cc->SetName("ws-cc");

  cc->SetHostAndPort(host, port);

  lso->SetName("ws-lso");

  lso->SetClientCallback(cbk);

  D->AddEventListener(cc)->AddEventListener(lso);

  return lso;
}

auto make_http_client(const std::string& host, int port)
{
  auto sock = std::make_shared<CDeviceSocket>();
  auto http = std::make_shared<CProtocolHTTP>();    

  sock->SetHostAndPort(host, port);

  sock->SetName("http-socket");

  http->SetName("http-protocol");

  D->AddEventListener(sock)->AddEventListener(http);

  return http;
}

NS_END

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
