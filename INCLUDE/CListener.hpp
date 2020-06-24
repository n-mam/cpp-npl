#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <functional>

#include <CSubject.hpp>

namespace NPL {

using TListenerOnConnect = std::function<void (void)>;
using TListenerOnRead = std::function<void (const uint8_t *b, size_t n)>;
using TListenerOnWrite = std::function<void (const uint8_t *b, size_t n)>;
using TListenerOnDisconnect = std::function<void (void)>;
using TListenerOnAccept = std::function<void (void)>;

class CListener : public CSubject<uint8_t, uint8_t>
{
  public:

    CListener(
      TListenerOnConnect cbkConnect = nullptr,
      TListenerOnRead cbkRead = nullptr,
      TListenerOnWrite cbkWrite = nullptr,
      TListenerOnDisconnect cbkDisconnect = nullptr,
      TListenerOnAccept cbkAccept = nullptr)
    {
      iCbkConnect = cbkConnect;
      iCbkRead = cbkRead;
      iCbkWrite = cbkWrite;
      iCbkDisconnect = cbkDisconnect;
      iCbkAccept = cbkAccept;
    }

    ~CListener(){}

    virtual void OnRead(const uint8_t *b, size_t n)
    {
      if (iCbkRead)
      {
        iCbkRead(b, n);
      }
    }

    virtual void OnWrite(const uint8_t *b, size_t n)
    {
      if (iCbkWrite)
      {
        iCbkWrite(b, n);
      }
    }

    virtual void OnConnect(void)
    {
      if (iCbkConnect)
      {
        iCbkConnect();
      }
    }

    virtual void OnAccept()
    {
      if (iCbkAccept)
      {
        iCbkAccept();
      }
    }

    virtual void OnDisconnect(void)
    {
      if (iCbkDisconnect)
      {
        iCbkDisconnect();
      }
    }

  protected:

    TListenerOnRead iCbkRead;
    TListenerOnWrite iCbkWrite;
    TListenerOnAccept iCbkAccept;
    TListenerOnConnect iCbkConnect;
    TListenerOnDisconnect iCbkDisconnect;
};

} //namespace NPL

#endif //LISTENER_HPP