#ifndef PROTOCOLWS_HPP
#define PROTOCOLWS_HPP

#include <Util.hpp>
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
        bool fRet = false;

        auto sock = GetTargetSocketDevice();

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
          std::cout << "websocket handshake done\n";
        }
      }
    }

    virtual bool IsMessageComplete(const std::vector<uint8_t>& b) override
    {
      bool fRet = false;

      if (iWsHandshakeDone)
      {
        fRet = IsMessageComplete(b.data(), b.size());
      }
      else
      {
        fRet = CProtocolHTTP::IsMessageComplete(b);
      }

      return fRet;
    }

    virtual bool IsMessageComplete(const uint8_t *b, size_t l)
    {
      bool fRet = false;

      if (l < 2) return fRet;

      uint8_t opcode = b[0] & 0x0F;

      bool bMasked = false;

      if (b[1] & 0x80)
      {
        bMasked = true;
      }

      size_t payloadLength = 0;
      size_t maskingKeyIndex = 0;

      unsigned char indicator = b[1] & 0x7F;

      if (indicator <= 125)
      { /*
         * if 0-125, that is the payload length
         */
        payloadLength = indicator;
        maskingKeyIndex = 2; /** third byte */
      }
      else if (indicator == 126 && l >= (2 + 2))
      { /*
         * If 126, the following 2 bytes interpreted as a 
         * 16-bit unsigned integer are the payload length
         */
        payloadLength = BTOL(b + 2, 2);
        maskingKeyIndex = 4;
      }
      else if (indicator == 127 && l >= (2 + 8))
      { /*
         * If 127, the following 8 bytes interpreted as a 
         * 64-bit unsigned integer (the most significant bit 
         * MUST be 0) are the payload length
         */
        payloadLength = BTOL(b + 2, 8);
        maskingKeyIndex = 10;
      }
      else
      {
        assert(false);
      }

      unsigned char maskingKey[4];

      if (bMasked && (l >= (maskingKeyIndex + 4)))
      {
        for (int i = 0; i < 4; i++)
        {
          maskingKey[i] = b[maskingKeyIndex + i];
        }
      }

      std::string payload;

      size_t payloadIndex = maskingKeyIndex + (bMasked ? 4 : 0);

      if ((payloadIndex + payloadLength) == l)
      {
        for (int i = 0; i < payloadLength; i++)
        {
          payload += b[payloadIndex + i] ^ maskingKey[(i % 4)];
        }

        std::cout << payload << "\n";

        fRet = true;
      }

      return fRet;
    }

    virtual bool ValidateClientHello(const std::vector<uint8_t>& m)
    {
      return true; //todo
    }

    virtual bool ValidateServerHello(const std::vector<uint8_t>& m)
    {
      return true; //todo
    }

    virtual bool SendClientHello(void)
    {
      return false; //todo
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