#include <CDispatcher.hpp>
#include <CDeviceSocket.hpp>
#include <CProtocolFTP.hpp>

namespace NPL 
{
  auto make_dispatcher(void)
  {
    auto D = std::make_shared<CDispatcher>();

    D->InitializeControl();

    return D;
  }

  auto D = make_dispatcher();
  
  auto make_ftp(const std::string& host, int port, FTPS ftps = FTPS::None)
  {
    auto cc = std::make_shared<CDeviceSocket>();
    auto ftp = std::make_shared<CProtocolFTP>();    

    cc->SetHostAndPort(host, port);

    ftp->SetFTPS(ftps);

    cc->SetName("cc");

    ftp->SetName("ftp");

    NPL::D->AddEventListener(cc)->AddEventListener(ftp);

    return ftp;
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
