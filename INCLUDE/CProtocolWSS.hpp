#ifndef PROTOCOLWS_HPP
#define PROTOCOLWS_HPP

#include <CProtocolHTTP.hpp>

class CProtocolWS : public CProtocolHTTP
{

  protected:

    virtual void StateMachine(const std::vector<uint8_t>& msg) override
    {

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

    bool iWsHandshakeDone = false;
};

#endif //PROTOCOLWS_HPP