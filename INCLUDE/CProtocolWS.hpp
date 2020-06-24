#ifndef PROTOCOLWS_HPP
#define PROTOCOLWS_HPP

#include <CMessage.hpp>
#include <Encryption.hpp>
#include <CProtocolHTTP.hpp>

NS_NPL

class CProtocolWS : public CProtocolHTTP
{
  protected:

    virtual void StateMachine(const std::vector<uint8_t>& m) override
    {
      if (!iWsHandshakeDone)
      {
        auto sock = GetTargetSocketDevice();

        assert(sock);

        bool fRet = false;

        if (sock->IsClientSocket())
        {
          fRet = ValidateServerHello(m);
        }
        else
        {
          fRet = ValidateClientHello(m);
          
          if (fRet)
          {
            fRet = SendServerHello(m);
          }
        }

        if (fRet)
        {
          iWsHandshakeDone = true;
        }
      }
    }

    virtual bool IsMessageComplete(const std::vector<uint8_t>& b) override
    {
      bool fRet = false;

      if (iWsHandshakeDone)
      {
        fRet = IsWSMessageComplete(b);
      }
      else
      {
        fRet = CProtocolHTTP::IsMessageComplete(b);
      }

      return fRet;
    }

    virtual bool IsWSMessageComplete(const std::vector<uint8_t>& b)
    {
      return false;
    }

    virtual bool ValidateClientHello(const std::vector<uint8_t>& m)
    {
      return true; //todo
    }

    virtual bool ValidateServerHello(const std::vector<uint8_t>& m)
    {
      return true; //todo
    }

    virtual bool SendServerHello(const std::vector<uint8_t>& m)
    {
      CHTTPMessage cHello(m);

      auto key = cHello.GetHeader("Sec-WebSocket-Key");

      assert(key.size());

      key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

      unsigned char hash[20] = { '\0' };
      unsigned int hashlen;

      MessageDigest(
        (const unsigned char *) key.c_str(), 
        key.size(),
        hash,
        &hashlen);

      unsigned char base64[128] = { '\0' };

      Base64Encode(base64, hash, hashlen);

      std::stringstream sHello;

      sHello << "HTTP/1.1 101 Switching Protocols\r\n";
      sHello << "Upgrade: websocket\r\n";
      sHello << "Connection: Upgrade\r\n";
      sHello << "Sec-WebSocket-Accept: " << base64 << "\r\n";      
      sHello << "\r\n";

      Write((uint8_t *) sHello.str().c_str(), sHello.str().size(), 0);

      return true; //todo
    }

    virtual bool SendClientHello(void)
    {
      return false; //fixme
    }

    virtual void OnAccept(void) override
    {
      auto sock = GetTargetSocketDevice();

      if (sock)
      {
        auto aso = std::make_shared<CProtocolWS>();

        sock->iConnectedClient->AddEventListener(aso);
      }
    }

    virtual void OnConnect(void) override
    {
      SendClientHello();
    }

    bool iWsHandshakeDone = false;
};

NS_END

#endif //PROTOCOLWS_HPP