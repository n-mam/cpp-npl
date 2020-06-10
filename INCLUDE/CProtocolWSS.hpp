#ifndef PROTOCOLWS_HPP
#define PROTOCOLWS_HPP

#include <CProtocol.hpp>

class CProtocolWS : public CProtocol<uint8_t, uint8_t>
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