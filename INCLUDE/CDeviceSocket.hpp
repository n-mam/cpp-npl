#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <CDevice.hpp>

#include <memory>
#include <string>

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace NPL {

using TOnHandshake = std::function<void (void)>;

enum ESocketFlags : uint8_t
{
  ESocketInvalid = 0,
  EClientSocket,
  EListeningSocket,
  EAcceptedSocket,
};

class CDeviceSocket : public CDevice
{
  public:

    CDeviceSocket()
    {
      iDevicetype = EDeviceType::EDevSock;
      iFD = (FD) socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    CDeviceSocket(FD aSocket)
    {
      iFD = aSocket;
    }

    ~CDeviceSocket()
    {
      StopSocket();
      shutdown((SOCKET)iFD, 0); //sd_recv
      closesocket((SOCKET)iFD);
      if (ssl) SSL_free(ssl);
      std::cout << "~" << iName << " : shutdown socket sd_recv.\n";
    }

    virtual void StopSocket(void)
    {
      if (!iStopped)
      {
        if (ssl)
        {
          int flag = SSL_get_shutdown(ssl);

          if (!(flag & SSL_SENT_SHUTDOWN))
          {
            int rc = SSL_shutdown(ssl);
            std::cout << iName << " StopSocket() : ssl_shutdown rc : " << rc << "\n";          
            UpdateWBIO();
          }
        }

        shutdown((SOCKET)iFD, 1); //sd_send
        
        iStopped = true;

        std::cout << iName << " StopSocket() : shutdown socket sd_send.\n";
      }
    }

    virtual void StartSocketClient(void)
    {
      assert(iHost.size() && iPort);

      iSocketFlags |= ESocketFlags::EClientSocket;

      #ifdef linux

        SetSocketBlockingEnabled(iFD, false);

        sockaddr_in sa;

        inet_pton(AF_INET, iHost.c_str(), &sa.sin_addr);
        sa.sin_port = htons(iPort);
        sa.sin_family = AF_INET;

        int rc = connect((SOCKET)iFD, (const sockaddr *) &sa, sizeof(sa));

      #else

        struct sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_family = AF_INET;
        addr.sin_port = 0;

        int rc = bind((SOCKET)iFD, (SOCKADDR*) &addr, sizeof(addr));

        assert(rc == 0);

        Context *ctx = (Context *) calloc(1, sizeof(Context));

        ctx->type = EIOTYPE::CONNECT;

        sockaddr_in sa;
        ZeroMemory(&sa, sizeof(sa));
        sa.sin_family = AF_INET;
        inet_pton(AF_INET, iHost.c_str(), &sa.sin_addr);
        sa.sin_port = htons(iPort);

        auto ConnectEx = GetExtentionPfn(WSAID_CONNECTEX, iFD);

        bool fRet = ((LPFN_CONNECTEX)ConnectEx)(
          (SOCKET)iFD,
          (SOCKADDR*) &sa,
          sizeof(sa), 
          NULL, 0, NULL,
          (LPOVERLAPPED)ctx);

        if (fRet)
        {
          #ifdef WIN32
          setsockopt((SOCKET)iFD, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0 );
          #endif          
          OnConnect();
        }
        else if (WSAGetLastError() == ERROR_IO_PENDING)
        {

        }
        else
        {
          std::cout << "ConnectEx failed : " << WSAGetLastError() << "\n";
        }

      #endif
    }

    virtual void StartSocketServer(void)
    {
      assert(iPort);

      sockaddr_in sa;

      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr=INADDR_ANY;
      sa.sin_port = htons(iPort);

      int fRet = bind((SOCKET)iFD, (const sockaddr *) &sa, sizeof(sa));
      assert (fRet == 0);

      fRet = listen((SOCKET)iFD, SOMAXCONN);
      assert(fRet == 0);

      iSocketFlags |= ESocketFlags::EListeningSocket;

      #ifdef WIN32

        auto AcceptEx = GetExtentionPfn(WSAID_ACCEPTEX, iFD);

        uint8_t *b = (uint8_t *) calloc(1, 2 * (sizeof(SOCKADDR_STORAGE) + 16) + sizeof(Context));

        ((Context *)b)->type = EIOTYPE::ACCEPT;

        iAS = (FD) socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        DWORD bytesReceived;

        bool rc = ((LPFN_ACCEPTEX)AcceptEx)(
          (SOCKET)iFD,
          (SOCKET)iAS,
          b + sizeof(Context),
          0,
          sizeof(SOCKADDR_STORAGE) + 16,
          sizeof(SOCKADDR_STORAGE) + 16,
          &bytesReceived,
          (LPOVERLAPPED)b);
      #endif
    }

    virtual bool SetSocketBlockingEnabled(FD sock, bool blocking)
    {
      bool fret = false;

      #ifdef linux
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl((SOCKET)sock, F_SETFL, flags) == 0) ? true : false;
      #else
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket((SOCKET)sock, FIONBIO, &mode) == 0) ? true : false;
      #endif
    }

    virtual bool IsClientSocket(void)
    {
      return (iSocketFlags & ESocketFlags::EClientSocket);
    }

    virtual bool IsListeningSocket(void)
    {
      return (iSocketFlags & ESocketFlags::EListeningSocket);
    }

    virtual bool IsAcceptedSocket(void)
    {
      return (iSocketFlags & ESocketFlags::EAcceptedSocket);
    }

    virtual void SetHostAndPort(const std::string& aHostname, int aPort)
    {
      iHost = aHostname;
      iPort = aPort;
    }

    virtual void CheckPeerSSLShutdown()
    {
      int flag = SSL_get_shutdown(ssl);

      if (flag & SSL_RECEIVED_SHUTDOWN)
      {
        std::cout << iName << " CheckPeerSSLShutdown : SSL_RECEIVED_SHUTDOWN, flags : " << flag << "\n";

        if (!(flag & SSL_SENT_SHUTDOWN))
        {
          StopSocket();
        }
      }
    }

    virtual void UpdateWBIO()
    {
      CheckPeerSSLShutdown();

      int pending = BIO_pending(wbio);

      uint8_t buf[2048];

      if (pending)
      {
        int rc = BIO_read(wbio, buf, pending);

        CDevice::Write(buf, pending);
      }
    }

    virtual void InitializeSSL(TOnHandshake cbk = nullptr)
    {
      iOnHandShake = cbk;
      ctx = SSL_CTX_new(TLS_client_method());
      SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
      ssl = SSL_new(ctx);
      rbio = BIO_new(BIO_s_mem());
      wbio = BIO_new(BIO_s_mem());
      SSL_set_bio(ssl, rbio, wbio);

      if (IsClientSocket())
      {
        SSL_set_connect_state(ssl);

        SSL_do_handshake(ssl);

        UpdateWBIO();
      }
      else
      {
        SSL_set_accept_state(ssl);
      }
    }

    virtual void OnAccept(SPCSubject subject)
    {
      assert(IsListeningSocket());
      assert(subject == nullptr);

      #ifdef WIN32
      setsockopt((SOCKET)iAS, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(iFD), sizeof(iFD));
      #endif

      subject = std::make_shared<CDeviceSocket>(iAS);

      {
        auto sock = std::dynamic_pointer_cast<CDeviceSocket>(subject);
        sock->iSocketFlags |= ESocketFlags::EAcceptedSocket;
      }

      subject->SetName("AS");

      auto D = GetDispatcher();

      D->AddEventListener(subject);

      CDevice::OnAccept(subject);
    }

    virtual void OnConnect() override
    {
      CDevice::OnConnect();
      #ifdef WIN32
      CDevice::Read();
      setsockopt((SOCKET)iFD, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0 );
      #endif
    }

    virtual void OnRead(const uint8_t *b, size_t n) override
    {
      #ifdef WIN32
      CDevice::Read();
      #endif

      if (ssl)
      {
        int rc = BIO_write(rbio, b, n);

        assert(rc == n);

        if (!iHandshakeDone)
        {
          rc = SSL_do_handshake(ssl);

          if (rc == 1)
          {
            iHandshakeDone = true;

            if (iOnHandShake)
            {
              iOnHandShake();
            }
          }
        }

        if (iHandshakeDone)
        {
          rc = SSL_read(ssl, iRBuf, 1024);
        }

        UpdateWBIO();

        if (rc <= 0)
        {
          return;
        }
        else
        {
          b = iRBuf, n = rc;
        }
      }

      CDevice::OnRead(b, n);
    }

    virtual void OnWrite(const uint8_t *b, size_t n) override
    {
      if (ssl)
      {
        if (!iHandshakeDone)
        {
          SSL_do_handshake(ssl);
          return;
        }
      }

      CDevice::OnWrite(b, n);
    }

    virtual void * Read(const uint8_t *b = nullptr, size_t l = 0, uint64_t o = 0) override
    {
      #ifdef linux
      if (IsListeningSocket())
      {
        return CreateAcceptSocketContext();
      }
      #endif

      return CDevice::Read(b, l, o);
    }

    virtual void Write(const uint8_t *b = nullptr, size_t l = 0, uint64_t o = 0) override
    {
      if (ssl)
      {
        SSL_write(ssl, b, l);
        UpdateWBIO();
      }
      else
      {
        CDevice::Write(b, l);
      }
    }

    #ifdef linux
    virtual void * CreateAcceptSocketContext(void)
    {
      Context *ctx = (Context *) calloc(1, sizeof(Context));
      struct sockaddr asa;
      socklen_t len = sizeof(struct sockaddr);
      iAS = accept(iFD, &asa, &len);
      SetSocketBlockingEnabled(iAS, false);
      ctx->type = EIOTYPE::ACCEPT;
      return ctx;
    }
    #endif
    
  private:

    FD iAS;

    int iPort = 0;

    std::string iHost = "";

    uint32_t iSocketFlags = ESocketFlags::ESocketInvalid;

    bool iStopped = false;

    SSL_CTX *ctx = nullptr;

    SSL *ssl = nullptr;

    BIO *rbio = nullptr;

    BIO *wbio = nullptr;

    uint8_t iRBuf[1024];

    bool iHandshakeDone = false;

    TOnHandshake iOnHandShake = nullptr;

    #ifdef WIN32
    void * GetExtentionPfn(GUID guid, FD fd)
    {
      void *pfn;
      DWORD n;

      int rc = WSAIoctl(
        (SOCKET)fd,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guid, sizeof(guid),
        &pfn, sizeof(pfn),
        &n, NULL, NULL);

      assert(rc == 0);

      return pfn;
    }
    #endif
};

using SPCDeviceSocket = std::shared_ptr<CDeviceSocket>;

} //namespace NPL

#ifdef WIN32
 WSADATA wsaData;
 int winsockinit = WSAStartup(MAKEWORD(2, 2), &wsaData);
 void * GetExtentionPfn(GUID guid, FD fd); 
#endif 

#endif //SOCKET_HPP