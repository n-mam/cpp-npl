#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <CSubject.hpp>

#include <map>
#include <functional>

NS_NPL

template <typename T1 = uint8_t, typename T2 = uint8_t>
class CProtocol : public CSubject<T1, T2>
{
  public:

    using SPCProtocol = std::shared_ptr<CProtocol<uint8_t, uint8_t>>;

    using TOnClientMessageCbk = 
      std::function<
        void (SPCProtocol, const std::string&)
      >;

    CProtocol() = default;

    virtual ~CProtocol() {}

    virtual void StartClient(void)
    {
      auto sock = GetTargetSocketDevice();

      if (sock)
      {
        sock->StartSocketClient();
      }
    }

    virtual void StartServer(void)
    {
      auto sock = GetTargetSocketDevice();

      if (sock)
      {
        sock->StartSocketServer();
      }
    }

    virtual void Stop(void)
    {
      auto sock = GetTargetSocketDevice();

      if (sock)
      {
        sock->StopSocket();
      }      
    }

    virtual size_t GetMessageCount(void)
    {
      return iMessages.size();
    }

    virtual void SetCredentials(const std::string& user, const std::string& pass)
    {
      iUserName = user;
      iPassword = pass;
    }

    virtual void SetClientCallback(TOnClientMessageCbk cbk)
    {
      iClientMessageCallback = cbk;
    }

    virtual void SendProtocolMessage(const uint8_t *message, size_t len)
    {
    }

    virtual void OnConnect(void) override
    {
      iProtocolState = "CONNECTED";
    }

    virtual void OnDisconnect(void) override
    {
      iProtocolState = "DISCONNECTED";
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

    virtual TLS GetChannelTLS(SPCSubject channel)
    {
      auto sock =  std::dynamic_pointer_cast<CDeviceSocket>(channel);

      if (sock)
      {
        return sock->GetTLS();
      }

      return TLS::NO;
    }

  protected:

    virtual bool IsMessageComplete(const std::vector<T1>& b) = 0;

    virtual void StateMachine(const std::vector<T1>& buffer)
    {
    }

    virtual SPCDeviceSocket GetTargetSocketDevice(void)
    {
      auto target = (this->iTarget).lock();

      if (target)
      {
        auto sock = std::dynamic_pointer_cast<CDeviceSocket>(target);

        return sock;
      }

      return nullptr;
    }

    std::string iUserName;

    std::string iPassword;

    std::vector<T1> iBuffer;

    std::vector<std::vector<T2>> iMessages;

    TOnClientMessageCbk iClientMessageCallback = nullptr;

    std::string iProtocolState = "CONNECTING";
};

using SPCProtocol = std::shared_ptr<CProtocol<uint8_t, uint8_t>>;

using TOnClientMessageCbk = 
  std::function<
    void (SPCProtocol, const std::string&)
  >;

NS_END

#endif //PROTOCOL_HPP