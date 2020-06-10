#include <CDispatcher.hpp>
#include <CDeviceSocket.hpp>
#include <CProtocolFTP.hpp>
#include <CProtocolWSS.hpp>

namespace NPL 
{
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

  auto make_websocket(const std::string& host, int port, TLS tls = TLS::NO)
  {
    auto cc = std::make_shared<CDeviceSocket>();
    auto ws = std::make_shared<CProtocolWS>(); 

    cc->SetHostAndPort(host, port);

    cc->SetTLS(tls);

    cc->SetName("ws-cc");

    D->AddEventListener(cc)->AddEventListener(ws);

    return ws;
  }

} //namespace NPL

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
