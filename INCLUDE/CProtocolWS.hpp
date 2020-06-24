#ifndef PROTOCOLWS_HPP
#define PROTOCOLWS_HPP

#include <CMessage.hpp>
#include <CProtocolHTTP.hpp>

NS_NPL

class CProtocolWS : public CProtocolHTTP
{
  protected:

    virtual void StateMachine(const std::vector<uint8_t>& m) override
    {
      if (!iWsHandshakeDone)
      {
        auto sock = GetTargetSocketDevice();

        if (sock)
        {
          if (sock->IsClientSocket())
          {
            BuildClientHello();
          }
          else
          {
            BuildServerHello(m);
          }
        }
      }
    }

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

    virtual void BuildClientHello()
    {

    }

    virtual void BuildServerHello(const std::vector<uint8_t>& m)
    {
      CHTTPMessage clientHello(m);
    }

    virtual void OnAccept(void) override
    {
      auto sock = GetTargetSocketDevice();

      if (sock)
      {
        auto aso = std::make_shared<CProtocolWS>();

        sock->iConnectedClient->AddEventListener(aso);
      }
    }

    bool iWsHandshakeDone = false;
};

NS_END

#endif //PROTOCOLWS_HPP