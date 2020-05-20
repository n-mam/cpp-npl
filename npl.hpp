#include <CDispatcher.hpp>
#include <CDeviceSocket.hpp>
#include <CProtocolFTP.hpp>

namespace npl 
{
  auto D = std::make_shared<CDispatcher>();

  auto make_ftp(const std::string& host, int port, const std::string& username, const std::string& password)
  {
    auto cc = std::make_shared<CDeviceSocket>();
    auto ftp = std::make_shared<CProtocolFTP>();    

    cc->SetName("cc");

    ftp->SetName("ftp");

    D->AddEventListener(cc)->AddEventListener(ftp);

    ftp->SetCredentials(username, password);

    cc->StartSocketClient(host, port);

    return ftp;
  }

} //namespace npl

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
