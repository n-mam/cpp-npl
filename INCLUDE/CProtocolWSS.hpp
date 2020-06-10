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
      return false;
    }
};

#endif //PROTOCOLWS_HPP