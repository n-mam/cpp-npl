#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <CDevice.hpp>

#include <memory>
#include <string>

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

    void StartSocketClient(std::string host, int port)
    {
      iHost = host;
      iPort = port;

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

    void StartSocketServer(int port)
    {
      iPort = port;

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

      CDevice::OnRead(b, n);
    }

    virtual void OnWrite(const uint8_t *b, size_t n) override
    {
      CDevice::OnWrite(b, n);
    }

  private:

    int iPort = 0;

    std::string iHost = "";

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