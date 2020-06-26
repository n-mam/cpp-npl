#ifndef PROTOCOLWS_HPP
#define PROTOCOLWS_HPP

#include <Util.hpp>
#include <CMessage.hpp>
#include <Encryption.hpp>
#include <CProtocolHTTP.hpp>

NS_NPL

class CProtocolWS : public CProtocolHTTP
{
  public:

    virtual void SendProtocolMessage(const uint8_t *data, size_t len) override
    {
      unsigned char frame[10];
      int frameLength = 0;

      /* 1000001 */
      frame[0] = 0x81;
      frameLength++;

      if (len <= 125)
      {
        frame[1] = (unsigned char) len;
      }
      else if (len <= 0xFFFF)
      {
        frame[1] = 126;
        frameLength += 2;
        LTOB(len, frame + 2, 2);
      }
      else if (len > 65536)
      {
        frame[1] = 127;
        frameLength += 8;
        LTOB(len, frame + 2, 8);
      }
      else
      {
        // what now ?
      }

      frameLength++;

      std::string message;

      message.insert(0, (char *) frame, frameLength);
      message.insert(frameLength, (char *) data, len);

      Write((uint8_t *) message.data(), message.size(), 0);
    }

  protected:

    bool iWsHandshakeDone = false;

    virtual void StateMachine(SPCMessage m) override
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
        }
      }
      else
      {
        if (iClientMessageCallback)
        {
          iClientMessageCallback(
            std::dynamic_pointer_cast<CProtocol>(shared_from_this()),
            m->GetPayloadString()
          );
        }
      }
    }

    virtual SPCMessage IsMessageComplete(const std::vector<uint8_t>& b) override
    {
      if (iWsHandshakeDone)
      {
        return IsMessageComplete(b.data(), b.size());
      }
      else
      {
        return CProtocolHTTP::IsMessageComplete(b);
      }
    }

    virtual SPCMessage IsMessageComplete(const uint8_t *b, size_t l)
    {
      SPCMessage fRet = nullptr;

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
        return fRet;
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

        fRet = std::make_shared<CWSMessage>(payload); //fixme
      }

      return fRet;
    }

    virtual bool ValidateClientHello(SPCMessage m)
    {
      return true; //todo
    }

    virtual bool ValidateServerHello(SPCMessage m)
    {
      return true; //todo
    }

    virtual bool SendClientHello(void)
    {
      return false; //todo
    }

    virtual bool SendServerHello(SPCMessage m)
    {
      auto cHello = std::dynamic_pointer_cast<CHTTPMessage>(m);

      auto key = cHello->GetHeader("Sec-WebSocket-Key");

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

        aso->SetClientCallback(iClientMessageCallback);

        sock->iConnectedClient->AddEventListener(aso);
      }
    }

    virtual void OnConnect(void) override
    {
      SendClientHello();
    }
};

NS_END

#endif //PROTOCOLWS_HPP