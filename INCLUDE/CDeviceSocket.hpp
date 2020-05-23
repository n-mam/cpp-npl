#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <CDevice.hpp>

#include <memory>
#include <string>

#include <openssl/ssl.h>
#include <openssl/err.h>

class CDeviceSocket : public CDevice
{
  public:

    CDeviceSocket()
    {
      iFD = (FD) socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    ~CDeviceSocket()
    {
      shutdown((SOCKET)iFD, 0);
      closesocket((SOCKET)iFD);
    }

    void Shutdown(void)
    {
      shutdown((SOCKET)iFD, 1);      
    }

    void StartSocketClient(void)
    {
      assert(iHost.size() && iPort);

      #ifdef linux

        SetSocketBlockingEnabled(false);

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

        ctx->type = EIOTYPE::INIT;

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

    void StartSocketServer(void)
    {
      assert(iPort);

      sockaddr_in sa;

      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr=INADDR_ANY;
      sa.sin_port = htons(iPort);

      int fRet = bind((SOCKET)iFD, (const sockaddr *) &sa, sizeof(sa));

      if (fRet == 0)
      {
        fRet = listen((SOCKET)iFD, SOMAXCONN);
      }

      #ifdef WIN32
        auto AcceptEx = GetExtentionPfn(WSAID_ACCEPTEX, iFD);

        FD as = (FD) socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        uint8_t *b = (uint8_t *) calloc(1, 2 * (sizeof(SOCKADDR_STORAGE) + 16) + sizeof(Context));

        DWORD bytesReceived;

        bool rc = ((LPFN_ACCEPTEX)AcceptEx)(
          (SOCKET)iFD, (SOCKET)as, b, 0,
          sizeof(SOCKADDR_STORAGE) + 16,
          sizeof(SOCKADDR_STORAGE) + 16,
          &bytesReceived,
          (LPOVERLAPPED)(b + (2 * (sizeof(SOCKADDR_STORAGE) + 16))));
      #endif
    }

    bool SetSocketBlockingEnabled(bool blocking)
    {
      bool fret = false;

      #ifdef linux
        int flags = fcntl(iFD, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl((SOCKET)iFD, F_SETFL, flags) == 0) ? true : false;
      #else
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket((SOCKET)iFD, FIONBIO, &mode) == 0) ? true : false;
      #endif
    }

    bool IsClientSocket(void)
    {
      bool fRet = false;

      if (iHost.size() && iPort)
      {
        fRet = true;
      }

      return fRet;
    }

    void SetHostAndPort(const std::string& aHostname, int aPort)
    {
      iHost = aHostname;
      iPort = aPort;
    }

    virtual void OnConnect() override
    {
      CDevice::OnConnect();
      #ifdef WIN32
      setsockopt((SOCKET)iFD, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0 );
      #endif      
    }

    virtual void OnRead(const uint8_t *b, size_t n) override
    {
      assert(n);

      CDevice::Read();

      if (ssl)
      {
        BIO_write(rbio, b, n);

        int rc;

        if (!iHandshakeDone)
        {
          rc = SSL_do_handshake(ssl);
          
          if (rc == 1)
          {
            iHandshakeDone = true;
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

    virtual void Write(const uint8_t *b = nullptr, size_t l = 0, uint64_t o = 0) override
    {
      if(ssl)
      {
        int rc = SSL_write(ssl, b, l);

        UpdateWBIO();
      }
      else
      {
        CDevice::Write();
      }
    }

    void UpdateWBIO()
    {
      int l = BIO_pending(wbio);

      uint8_t buf[1024];

      if (l)
      {
        int rc = BIO_read(wbio, buf, l);

        CDevice::Write(buf, l);
      }
    }

    void InitializeSSL(void)
    {
      ctx = SSL_CTX_new(TLSv1_2_client_method());
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

  private:

    int iPort = 0;

    std::string iHost = "";

    SSL_CTX * ctx = nullptr;

    SSL * ssl = nullptr;

    BIO * rbio = nullptr;

    BIO * wbio = nullptr;

    bool iHandshakeDone = false;

    uint8_t iRBuf[1024];

    uint8_t iWBuf[1024];

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

#ifdef WIN32
 WSADATA wsaData;
 int winsockinit = WSAStartup(MAKEWORD(2, 2), &wsaData);
 void * GetExtentionPfn(GUID guid, FD fd); 
#endif 

#endif //SOCKET_HPP