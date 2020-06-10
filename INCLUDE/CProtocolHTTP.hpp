#ifndef PROTOCOLHTTP_HPP
#define PROTOCOLHTTP_HPP

#include <CProtocol.hpp>

class CProtocolHTTP : public CProtocol<uint8_t, uint8_t>
{
  public:

  protected:

    virtual void StateMachine(const std::vector<uint8_t>& msg) override
    {

    }

    virtual bool IsMessageComplete(const std::vector<uint8_t>& b) override
    {
      bool firstLineReceived = false;
      bool headersReceived = false;
      bool bodyReceived = false;
      int bodyLength = 0;

      return firstLineReceived &&
             headersReceived && 
             (bodyLength ? bodyReceived : true);
    }


};

#endif //PROTOCOLHTTP_HPP