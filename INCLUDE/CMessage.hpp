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
  
    CMessage(const std::vector<uint8_t>& m)
    {
      iMessage = std::string((char *) m.data(), m.size());
    }

  protected:

    std::string iMessage;

    virtual void ParseMessage(void) { }
};

class CHTTPMessage : public CMessage
{
  public:

    CHTTPMessage(const std::vector<uint8_t>& m) : CMessage(m)
    {
      ParseMessage();
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

      while (std::getline(ss, line, '\r'))
      {
        size_t index = line.find(": ");
        
        std::string key, value;

        key = line.substr(0, index);
        
        value = line.substr(index + 2);

        SetHeader(key, value);
      }
    }
};

NS_END

#endif //MESSAGE_HPP