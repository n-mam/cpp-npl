#include <CDispatcher.hpp>
#include <CDeviceSocket.hpp>
#include <CProtocolFTP.hpp>

namespace NPL 
{
  auto D = std::make_shared<CDispatcher>();

  auto make_ftp(const std::string& host, int port, ESSL ftps = ESSL::None)
  {
    auto cc = std::make_shared<CDeviceSocket>();
    auto ftp = std::make_shared<CProtocolFTP>();    

    cc->SetHostAndPort(host, port);

    ftp->SetFTPSType(ftps);

    cc->SetName("cc");

    ftp->SetName("ftp");

    D->AddEventListener(cc)->AddEventListener(ftp);

    return ftp;
  }

  using TFileReadCbk = std::function<bool (const uint8_t *, size_t)>;

  auto make_file_reader(TFileReadCbk cbk, const std::string& aFile)
  {
    auto fDev = std::make_shared<CDevice>(aFile.c_str());

    auto observer = std::make_shared<CListener>(
      nullptr,
      [=] (const uint8_t *b, size_t n) {
        if (cbk(b, n))
        {
          fDev->Read();
        }
      },
      nullptr,
      [=] () {
        cbk(nullptr, 0);
      }
    );

    D->AddEventListener(fDev)->AddEventListener(observer);

    return fDev;
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
