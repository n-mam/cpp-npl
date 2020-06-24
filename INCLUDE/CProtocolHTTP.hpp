#ifndef PROTOCOLHTTP_HPP
#define PROTOCOLHTTP_HPP

#include <CProtocol.hpp>

class CProtocolHTTP : public CProtocol<uint8_t, uint8_t>
{
  public:

  protected:

    virtual bool IsMessageComplete(const std::vector<uint8_t>& b) override
    {
      bool firstLineReceived = false;
      bool headersReceived = false;
      bool bodyReceived = false;
      int bodyLength = 0;

      std::string message((char *) b.data(), b.size());

      if (message.find("\r\n") != std::string::npos)
      {
         firstLineReceived = true;

         if (message.find("\r\n\r\n") != std::string::npos)
         {
           headersReceived = true;

           if (message.find("Content-Length") != std::string::npos)
           {
             //bodyLength 
           }
         }
      }

      return firstLineReceived &&
             headersReceived && 
             (bodyLength ? bodyReceived : true);
    }


};

#endif //PROTOCOLHTTP_HPP