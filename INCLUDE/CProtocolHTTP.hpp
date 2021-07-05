#ifndef PROTOCOLHTTP_HPP
#define PROTOCOLHTTP_HPP

#include <CProtocol.hpp>

namespace NPL 
{
  class CHTTPMessage : public CMessage
  {
    protected:

    std::string iPayload;

    std::map<std::string, std::string> iHeaders;

    virtual void ParseMessage() override
    {
      bool firstLineReceived = false;
      bool headersReceived = false;
      bool bodyReceived = false;
      int bodyLength = 0;

      if (iMessage.find("\r\n") != std::string::npos)
      {
        firstLineReceived = true;

        size_t endofHeaders = iMessage.find("\r\n\r\n");

        if (endofHeaders != std::string::npos)
        {
          headersReceived = true;

          size_t pos = iMessage.find("Content-Length");

          if (pos != std::string::npos)
          {
            pos += strlen("Content-Length: ");

            bodyLength = std::stoi(
              std::string(
                iMessage,
                pos,
                iMessage.find("\r\n", pos) - pos - 1));

            if (bodyLength)
            {
              size_t total = endofHeaders + strlen("\r\n\r\n") + bodyLength;

              if (iMessage.size() == total)
              {
                bodyReceived = true;
                iPayload = iMessage.substr(endofHeaders + strlen("\r\n\r\n"), bodyLength);
              }
            }
          }
        }
      }

      if (firstLineReceived && headersReceived && 
          (bodyLength ? bodyReceived : true))
      {
        std::istringstream ss(iMessage);

        std::string line;

        while (std::getline(ss, line, '\n'))
        {
          line.pop_back();

          size_t index = line.find(": ");

          if (index != std::string::npos)
          {
            std::string key, value;

            key = line.substr(0, index);

            value = line.substr(index + 2);

            SetHeader(key, value);
          }
        }
      }

    }

    public:
 
    CHTTPMessage(const std::vector<uint8_t>& m) : CMessage(m)
    {
      ParseMessage();
    }

    virtual std::string GetHeader(const std::string& key)
    {
      return iHeaders[key];
    }

    virtual size_t HeaderCount(void)
    {
      return iHeaders.size();
    }

    virtual void SetHeader(const std::string& key, const std::string& value)
    {
      iHeaders[key] = value;
    }

    virtual size_t GetPayloadLength(void) override
    {
      auto& h = GetHeader("Content-Length");

      if (h.size())
      {
        return std::stoi(h);
      }

      return 0;
    }

    virtual const std::string& GetPayloadString(void) override
    {
      return iPayload;
    }

    virtual const char * GetPayloadBuffer(void) override
    {
      return GetPayloadString().c_str();
    }
  };

  using SPCHTTPMessage = std::shared_ptr<CHTTPMessage>;

  class CProtocolHTTP : public CProtocol<uint8_t, uint8_t>
  {
    public:

    void Post(const std::string& url, const std::string& body)
    {
      std::stringstream req;

      req << "POST " << url << " HTTP/1.1\r\n";
      req << "Host: 127.0.0.1\r\n";
      req << "Content-type: text/plain\r\n";
      req << "Content-length: " << body.size() << "\r\n";      
      req << "\r\n";
      req << body;

      Write((uint8_t *) req.str().c_str(), req.str().size(), 0);
    }

    protected:

    virtual void StateMachine(SPCMessage m) override
    {
      return;
    }

    virtual SPCMessage IsMessageComplete(const std::vector<uint8_t>& b) override
    {
      auto m = std::make_shared<CHTTPMessage>(b);

      if (m->HeaderCount())
      {
        return m;
      }

      return nullptr;
    }
  };

  using SPCProtocolHTTP = std::shared_ptr<CProtocolHTTP>;
}

#endif //PROTOCOLHTTP_HPP