#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <CDevice.hpp>
#include <CSubject.hpp>

#include <iostream>
#include <thread>

#ifdef linux
 #include <unistd.h>
 #include <sys/epoll.h>
#endif

class CDispatcher : public CSubject<uint8_t, uint8_t>
{
  public:

    CDispatcher()
    {
      #ifdef linux
        iEventPort = epoll_create1(0);
      #else
        iEventPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
      #endif

      if (!iWorker.joinable())
      {
        iWorker = std::thread(&CDispatcher::Worker, this);
      }
    }

    ~CDispatcher()
    {
      #ifdef linux

        iWorker.join();

        if (iEventPort >= 0)
        {
          close(iEventPort);
        }

      #else

        PostQueuedCompletionStatus(iEventPort, 0, 0, 0);

        iWorker.join();

        if (iEventPort != INVALID_HANDLE_VALUE)
        {
          CloseHandle(iEventPort);
        }
      #endif
    }

    virtual bool IsDispatcher(void) override
    {
      return true;
    }

    virtual const SPCSubject& AddEventListener(const SPCSubject& observer)
    {
      CSubject::AddEventListener(observer);

      const SPCDevice device = std::dynamic_pointer_cast<CDevice>(observer);

      bool fRet = true;

      #ifdef linux
        assert(iEventPort != -1);

        struct epoll_event e;

        e.events = EPOLLIN | EPOLLOUT | EPOLLONESHOT;
        e.data.ptr = device.get();

        int rc = epoll_ctl(iEventPort, EPOLL_CTL_ADD, device->iFD, &e);

        if (rc == -1)
        {
          fRet = false;
        }
      #else

        assert(iEventPort != INVALID_HANDLE_VALUE);

        HANDLE port =
          CreateIoCompletionPort(
            device->iFD,
            iEventPort,
            (ULONG_PTR) device.get(),
            0);

        assert(port);
    
        if (port == NULL)
        {
          fRet = false;
        }

      #endif

      return observer;
    }

  private:

    FD iEventPort;

    std::thread iWorker;

    void Worker(void)
    {
      while (true)
      {
        Context *ctx = nullptr;
        unsigned long n;

        #ifdef linux

          struct epoll_event e;

          int fRet = epoll_wait(iEventPort, &e, 1, -1);

          if (fRet == -1)
          {
            break;
          }
      
          void * k = e.data.ptr;

        #else

          LPOVERLAPPED o;
          ULONG_PTR k;

          bool fRet = GetQueuedCompletionStatus(iEventPort, &n, &k, &o, INFINITE);

          //std::cout << (void *)k << " fRet " << fRet << ", n " << n << "\n";

          if (!fRet)
          {
            std::cout << "GetQueuedCompletionStatus failed : " << GetLastError() << "\n";
            continue;
          }

          if (n == 0 && k == 0 && o == 0)
          {
            break;
          }

          ctx = (Context *) o;

        #endif

        ctx->n = n;

        std::lock_guard<std::recursive_mutex> lg(iLock);

        for (auto& observer : iObservers)
        {
          if ((void *)observer.get() == (void *)k)
          {
            if (ctx->type == EIOTYPE::READ)
            {
              if (ctx->n != 0)
              {
                observer->OnRead(ctx->b, ctx->n);
                free(ctx->b);
              }
              else
              {
                observer->OnDisconnect();
              }
            }
            else if (ctx->type == EIOTYPE::WRITE)
            {
              observer->OnWrite(ctx->b, ctx->n);
            }
            else if (ctx->type == EIOTYPE::INIT)
            {
              observer->OnConnect();
            }
            else
            {
              assert (false);
            }

            break;
          }
        }

        free(ctx);
      }
      std::cout << "Dispatcher thread returning\n";
    }    
};

using SPCDispatcher = std::shared_ptr<CDispatcher>;
using UPCDispatcher = std::unique_ptr<CDispatcher>;

#endif //DISPATCHER_HPP