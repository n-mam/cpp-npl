#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <map>
#include <string>
#include <vector>
#include <sstream>

NS_NPL

class CMessage
{
  public:
  
    CMessage(const uint8_t *b, size_t l)
    {
      iMessage = std::string((char *)b, l);
    }

    CMessage(const std::vector<uint8_t>& m)
    {
      iMessage = std::string((char *) m.data(), m.size());
    }

    virtual std::string GetPayloadString(void)
    {
      return iMessage;
    }

    virtual const char * GetPayloadBuffer(void)
    {
      return iMessage.c_str();
    }

    virtual size_t GetPayloadLength(void)
    {
      return iMessage.size();
    }

  protected:

    std::string iMessage;

    virtual void ParseMessage(void) { }
};

class CFTPMessage : public CMessage
{
  public:

    CFTPMessage(const std::vector<uint8_t>& m) : CMessage(m)
    {
    }  
};

class CHTTPMessage : public CMessage
{
  public:

    CHTTPMessage(const std::vector<uint8_t>& m) : CMessage(m)
    {
      ParseMessage();
    }

    virtual const char * GetPayload(void)
    {
      char *payload = nullptr;

      if (GetHeader("Content-Length").size())
      {
        return iMessage.c_str() + 
               iMessage.find("\r\n\r\n") + 
               strlen("\r\n\r\n");
      }

      return payload;
    }

    virtual size_t GetPayloadLength(void)
    {
      size_t fRet = 0;

      auto cl = GetHeader("Content-Length");

      if (cl.size())
      {
        return std::stoi(cl);
      }

      return fRet;
    }

    virtual std::string GetHeader(const std::string& key)
    {
      return iHeaders[key];
    }

    virtual void SetHeader(const std::string& key, const std::string& value)
    {
      iHeaders[key] = value;
    }

  protected:

    std::map<std::string, std::string> iHeaders;

    virtual void ParseMessage() override
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
};

class CWSMessage : public CMessage
{
  public:

    CWSMessage(const uint8_t *b, size_t l) : CMessage(b, l)
    {
      ParseMessage();
    }
    
  protected:

    virtual void ParseMessage() override
    {

    }
};

using SPCMessage = std::shared_ptr<CMessage>;
using SPCWSMessage = std::shared_ptr<CWSMessage>;
using SPCHTTPMessage = std::shared_ptr<CHTTPMessage>;

NS_END

#endif //MESSAGE_HPP