#ifndef PROTOCOLWS_HPP
#define PROTOCOLWS_HPP

#include <CProtocolHTTP.hpp>

class CProtocolWS : public CProtocolHTTP
{
  protected:

    virtual bool IsMessageComplete(const std::vector<uint8_t>& b) override
    {
      bool fRet = false;

      if (iWsHandshakeDone)
      {
        fRet = IsWSMessageComplete(b);
      }
      else
      {
        fRet = CProtocolHTTP::IsMessageComplete(b);
      }

      return fRet;
    }

    virtual bool IsWSMessageComplete(const std::vector<uint8_t>& b)
    {
      return false;
    }

    virtual void OnAccept(void) override
    {
      auto target = (this->iTarget).lock();

      if (target)
      {
        auto sock = std::dynamic_pointer_cast<CDeviceSocket>(target);

        if (sock)
        {
          auto aso = std::make_shared<CProtocolWS>();

          sock->iConnectedClient->AddEventListener(aso);
        }
      }
    }

    bool iWsHandshakeDone = false;
};

#endif //PROTOCOLWS_HPP