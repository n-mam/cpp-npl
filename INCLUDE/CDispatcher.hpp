#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <CDevice.hpp>
#include <CSubject.hpp>
#include <CListener.hpp>
#include <CDeviceSocket.hpp>

#include <thread>
#include <iostream>

#ifdef linux
 #include <unistd.h>
 #include <sys/epoll.h>
 #include <string.h>
#endif

using namespace NPL;

class CDispatcher : public CSubject<uint8_t, uint8_t>
{
  public:

    CDispatcher()
    {
      iName = "D";

      #ifdef linux
        iEventPort = epoll_create1(0);
      #else
        iEventPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
      #endif
      /**
       * Dispatcher control server
       */
      iDControl = std::make_shared<CDeviceSocket>();

      iDControl->SetHostAndPort("", 1234);

      this->AddEventListener(iDControl);

      if (!iWorker.joinable())
      {
        iWorker = std::thread(&CDispatcher::Worker, this);
      }

      iDControl->SetName("LS");

      iDControl->StartSocketServer();
      /**
       * Dispatcher client
       */
      iDClient = std::make_shared<CDeviceSocket>();

      iDClient->SetHostAndPort("127.0.0.1", 1234);

      this->AddEventListener(iDClient);

      iDClient->SetName("DCC");

      iDClient->StartSocketClient();      
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

    virtual const SPCSubject& AddEventListener(const SPCSubject& observer)
    {
      CSubject::AddEventListener(observer);

      const SPCDevice device = std::dynamic_pointer_cast<CDevice>(observer);

      #ifdef linux

        assert(iEventPort != -1);

        struct epoll_event e;

        e.events = EPOLLIN | EPOLLOUT;

        e.data.ptr = device.get();

        int rc = epoll_ctl(iEventPort, EPOLL_CTL_ADD, device->iFD, &e);

        assert(rc == 0);

      #endif

      #ifdef WIN32

        assert(iEventPort != INVALID_HANDLE_VALUE);

        HANDLE port =
          CreateIoCompletionPort(
            device->iFD,
            iEventPort,
            (ULONG_PTR) device.get(),
            0);

        assert(port);

      #endif

      return observer;
    }

  private:

    FD iEventPort;

    std::thread iWorker;

    SPCDeviceSocket iDControl;

    SPCDeviceSocket iDClient;

    void Worker(void)
    {
      while (true)
      {
        Context *ctx = nullptr;

        void *k = nullptr;

        unsigned long n;

        #ifdef linux

          struct epoll_event e;

          int fRet = epoll_wait(iEventPort, &e, 1, -1);

          if (fRet < 0 && errno == EINTR)
          {
            continue;
          }

          k = e.data.ptr;

        #endif

        #ifdef WIN32

          LPOVERLAPPED o;

          bool fRet = GetQueuedCompletionStatus(iEventPort, &n, (PULONG_PTR) &k, &o, INFINITE);

          if (!fRet)
          {
            std::cout << k  << " GQCS failed : " << GetLastError() << "\n";
          }

          if (n == 0 && k == 0 && o == 0)
          {
            break;
          }

          ctx = (Context *) o;

          ctx->n = n;

        #endif

        std::unique_lock<std::mutex> ul(iLock);

        for (auto& o : iObservers)
        {
          if (k == (void *)o.get())
          {
            #ifdef linux

            if ((e.events & EPOLLOUT) && !o->IsConnected())
            {
              o->OnConnect();
            }
            else if (e.events & EPOLLIN)
            {
              ctx = (NPL::Context *) o->Read();
            }

            if (!ctx)
            {
              std::cout << o->GetName() << " : " << (void *)k << " continuing..\n";
              continue;
            }

            #endif

            std::cout << unsigned(ctx->type) << " " << o->GetName() << " : " << (void *)k << " fRet " << fRet << ", n " << ctx->n << "\n";            

            ul.unlock();

            ProcessContext(o.get(), ctx);

            ul.lock();

            break;
          }
        }

        ProcessMarkRemoveAllListeners();
      }

      std::cout << "Dispatcher thread returning. Observers : " << iObservers.size() << "\n";
    }

    void ProcessContext(CSubject *o, Context *ctx)
    {
      assert(ctx);

      if (ctx->type == EIOTYPE::READ)
      {
        if (ctx->n != 0)
        {
          o->OnRead(ctx->b, ctx->n);
        }
        else
        {
          o->OnDisconnect();
        }
      }
      else if (ctx->type == EIOTYPE::WRITE)
      {
        o->OnWrite(ctx->b, ctx->n);
      }
      else if (ctx->type == EIOTYPE::CONNECT)
      {
        o->OnConnect();
      }
      else if (ctx->type == EIOTYPE::ACCEPT)
      {
        std::cout << "Client connected\n";

        #ifdef WIN32

        setsockopt((SOCKET)ctx->as, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(ctx->ls), sizeof(ctx->ls));

        auto as = std::make_shared<CDeviceSocket>(ctx->as);

        auto observer = std::make_shared<CListener>(
          nullptr,
          [this] (const uint8_t *b, size_t n) {
            OnDispatcherControlRead(b, n);
          },
          nullptr,
          nullptr
        );

        this->AddEventListener(as)->AddEventListener(observer);

        as->SetName("AS");

        #endif
      }
      else
      {
        assert (false);
      }

      if (ctx->b) 
      {
        free((void *)ctx->b);
      }

      free(ctx);
    }

    void OnDispatcherControlRead(const uint8_t *b, size_t n)
    {
      assert(n == sizeof(Context));

      Context *ctx = (Context *)b;

      ProcessContext((CSubject *)ctx->k, ctx);
    }

    virtual void QueuePendingContext(SPCSubject s, void *c) override
    {
      ((Context *)c)->k = s.get();

      iDClient->Write((const uint8_t *)c, sizeof(Context));
    }

    virtual bool IsDispatcher(void) override
    {
      return true;
    }
};

using SPCDispatcher = std::shared_ptr<CDispatcher>;
using UPCDispatcher = std::unique_ptr<CDispatcher>;

#endif //DISPATCHER_HPP;