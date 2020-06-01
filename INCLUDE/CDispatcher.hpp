#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include <CDevice.hpp>
#include <CSubject.hpp>

#include <iostream>
#include <thread>

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
      #ifdef linux
        iEventPort = epoll_create1(0);
      #else
        iEventPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
      #endif

      if (!iWorker.joinable())
      {
        iWorker = std::thread(&CDispatcher::Worker, this);
      }

      iName = "D";
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

    virtual void QueuePendingContext(SPCSubject s, void *c) override
    {
      iPendingContext.emplace_back(s, c);
    }

    virtual const SPCSubject& AddEventListener(const SPCSubject& observer)
    {
      CSubject::AddEventListener(observer);

      const SPCDevice device = std::dynamic_pointer_cast<CDevice>(observer);

      bool fRet = true;

      #ifdef linux
        assert(iEventPort != -1);

        struct epoll_event e;

        e.events = EPOLLIN | EPOLLOUT;

        e.data.ptr = device.get();

        int rc = epoll_ctl(iEventPort, EPOLL_CTL_ADD, device->iFD, &e);

        assert(rc == 0);

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

    std::list<std::tuple<SPCSubject, void *>> iPendingContext;

    void Worker(void)
    {
      while (true)
      {
        Context *ctx = nullptr;

        unsigned long n;

        #ifdef linux

          struct epoll_event e;

          int fRet = epoll_wait(iEventPort, &e, 1, -1);

          if (fRet < 0 && errno == EINTR)
          {
            continue;
          }

          void *k = e.data.ptr;

        #else

        LPOVERLAPPED o;
        ULONG_PTR k;

        bool fRet = GetQueuedCompletionStatus(iEventPort, &n, &k, &o, INFINITE);

        if (!fRet)
        {
          std::cout << (void *)k  << " GQCS failed : " << GetLastError() << "\n";
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
          if ((void *)o.get() == (void *)k)
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

            if (!ctx) continue;

            #endif

            std::cout << unsigned(ctx->type) << " " << o->GetName() << " : " << (void *)k << " fRet " << fRet << ", n " << ctx->n << "\n";            

            ul.unlock();

            ProcessContext(o, ctx);

            if (iPendingContext.size())
            {
              auto [ob, cx] = iPendingContext.front();

              ProcessContext(ob, (Context *)cx);

              iPendingContext.pop_front();
            }

            ul.lock();

            break;
          }
        }

        ProcessMarkRemoveAllListeners();
      }

      std::cout << "Dispatcher thread returning. Observers : " << iObservers.size() << "\n";
    }

    void ProcessContext(SPCSubject o, Context *ctx)
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
      else if (ctx->type == EIOTYPE::INIT)
      {
        o->OnConnect();
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
};

using SPCDispatcher = std::shared_ptr<CDispatcher>;
using UPCDispatcher = std::unique_ptr<CDispatcher>;

#endif //DISPATCHER_HPP;