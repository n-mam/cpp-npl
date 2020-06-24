#ifndef PROTOCOLHTTP_HPP
#define PROTOCOLHTTP_HPP

#include <CProtocol.hpp>

NS_NPL

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

      std::string m((char *) b.data(), b.size());

      if (m.find("\r\n") != std::string::npos)
      {
         firstLineReceived = true;

         size_t endofHeaders = m.find("\r\n\r\n");

         if (endofHeaders != std::string::npos)
         {
           headersReceived = true;

           size_t pos = m.find("Content-Length");

           if (pos != std::string::npos)
           {
             pos += strlen("Content-Length: ");

             bodyLength = std::stoi(
                std::string(m, 
                            pos, 
                            m.find("\r\n", pos) - pos - 1));

             if (bodyLength)
             {
               size_t total = endofHeaders + strlen("\r\n\r\n") + bodyLength;

               if (m.size() == total)
               {
                 bodyReceived = true;
               }
             } 
           }
         }
      }

      return firstLineReceived && 
             headersReceived && 
             (bodyLength ? bodyReceived : true);
    }

};

NS_END

#endif //PROTOCOLHTTP_HPP