#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <CSubject.hpp>

#include <functional>

using TProtocolEventCbk = std::function<void (const std::string&, char)>;

template <typename T1 = uint8_t, typename T2 = uint8_t>
class CProtocol : public CSubject<T1, T2>
{
  public:

    CProtocol() = default;

    virtual ~CProtocol() {}

    virtual void Start(void)
    {

    }

    virtual size_t GetMessageCount(void)
    {
      return iMessages.size();
    }

    virtual void SetStateChangeCallback(TProtocolEventCbk evtcbk)
    {
      iEventCallback = evtcbk;
    }

    virtual void SetCredentials(const std::string& user, const std::string& pass)
    {
      iUserName = user;
      iPassword = pass;
    }

    virtual void Stop(void)
    {
    }

    virtual void OnConnect(void) override
    {
      iProtocolState = "CONNECTED";
      NotifyState("CONNECTED", 'S');
    }

    virtual void OnDisconnect(void) override
    {
      iProtocolState = "DISCONNECTED";
      NotifyState("DISCONNECTED", 'S');
    }

    virtual void OnRead(const T1 *b, size_t n) override
    {
      for (size_t i = 0; i < n; i++)
      {
        iBuffer.push_back(b[i]);

        if (IsMessageComplete(iBuffer))
        {
          iMessages.push_back(iBuffer);
          StateMachine(iBuffer);
          CSubject<T1, T2>::OnRead(
            iBuffer.data(),
            iBuffer.size());
          iBuffer.clear();
        }
      }
    }
  
  protected:

    virtual void StateMachine(const std::vector<T1>& buffer) = 0;

    virtual bool IsMessageComplete(const std::vector<T1>& b) = 0;

    virtual void NotifyState(const std::string& command, char result)
    {
      if (iEventCallback)
      {
        iEventCallback(command, result);
      }
    }

    std::vector<T1> iBuffer;

    std::vector<std::vector<T2>> iMessages;

    std::string iUserName;
    std::string iPassword;

    TProtocolEventCbk iEventCallback = nullptr;

    std::string iProtocolState = "CONNECTING";
};

template <typename T1 = uint8_t, typename T2 = uint8_t>
using SPCProtocol = std::shared_ptr<CProtocol<T1, T2>>;

#endif //PROTOCOL_HPP