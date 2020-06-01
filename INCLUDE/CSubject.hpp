#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <mutex>
#include <atomic>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

template <typename T1, typename T2>
class CSubject : public std::enable_shared_from_this<CSubject<T1, T2>>
{
  public:

    using SPCSubject = std::shared_ptr<CSubject<T1, T2>>;
    using WPCSubject = std::weak_ptr<CSubject<T1, T2>>;

    CSubject() = default;

    virtual ~CSubject()
    {
      std::lock_guard<std::mutex> lg(iLock);
      RemoveAllEventListenersInternal();
      std::cout << "~" + iName + "\n";
    }

    virtual void SetTarget(const WPCSubject& target)
    {
      std::lock_guard<std::mutex> lg(iLock);
      iTarget = target;
    }

    virtual void QueuePendingContext(SPCSubject s, void *c)
    {
      auto target = iTarget.lock();

      if (target)
      {
        return target->QueuePendingContext(s, c);
      }      
    }

    virtual void * Read(const uint8_t *b = nullptr, size_t l = 0, uint64_t o = 0)
    {
      std::lock_guard<std::mutex> lg(iLock);

      auto target = iTarget.lock();

      if (target)
      {
        return target->Read(b, l, o);
      }

      return nullptr;
    }

    virtual int64_t Write(const uint8_t *b = nullptr, size_t l = 0, uint64_t o = 0)
    {
      std::lock_guard<std::mutex> lg(iLock);

      auto target = iTarget.lock();

      if (target)
      {
        return target->Write(b, l, o);
      }

      return -1;
    }

    virtual void OnConnect(void)
    {
      std::lock_guard<std::mutex> lg(iLock);
      iConnected = true;
      NotifyConnect();
    }

    virtual void OnRead(const T1 *b, size_t n)
    {
      std::lock_guard<std::mutex> lg(iLock);
      NotifyRead((const T2 *)b, n);
    }

    virtual void OnWrite(const T1 *b, size_t n)
    {
      std::lock_guard<std::mutex> lg(iLock);
      NotifyWrite(b, n);
    }

    virtual void OnDisconnect(void)
    {
      std::lock_guard<std::mutex> lg(iLock);
      iConnected = false;
      NotifyDisconnect();
      MarkRemoveAllListeners();
      MarkRemoveSelfAsListener();  
    }

    virtual const SPCSubject& AddEventListener(const SPCSubject& observer)
    {
      std::lock_guard<std::mutex> lg(iLock);
      iObservers.push_back(observer);
      observer->SetTarget(this->weak_from_this());
      return observer;
    }

    virtual void RemoveEventListener(const SPCSubject& observer)
    {
      std::lock_guard<std::mutex> lg(iLock);
      RemoveEventListenerInternal(observer);
    }

    virtual void RemoveAllEventListeners(void)
    {
      std::lock_guard<std::mutex> lg(iLock);
      RemoveAllEventListenersInternal();
    }

    virtual void MarkRemoveAllListeners(void)
    {
      iMarkRemoveAllListeners = true;
    }

    virtual void MarkRemoveSelfAsListener(void)
    {
      iMarkRemoveSelfAsListener = true;
    }

    virtual bool IsMarkRemoveSelfAsListener(void)
    {
      return iMarkRemoveSelfAsListener;
    }

    virtual bool IsConnected(void)
    {
      return iConnected;
    }

    virtual bool IsDispatcher(void)
    {
      return false;
    }

    SPCSubject GetDispatcher(void)
    {
       auto target = iTarget;

       while (true)
       {
         auto sp = target.lock();

         if (sp)
         {
           if (sp->IsDispatcher())
           {
             return sp;
           }
           else
           {
             target = sp->iTarget;
           }
         }
         else
         {
           return nullptr;
         }
       } 
    }

    virtual std::string GetName(void)
    {
      return iName;
    }

    virtual void SetName(const std::string& aName)
    {
      iName = aName;
    }
    
  protected:

    std::mutex iLock;

    WPCSubject iTarget;

    bool iConnected = false;

    std::string iName = "Subject";

    bool iMarkRemoveAllListeners = false;

    bool iMarkRemoveSelfAsListener = false;

    std::vector<SPCSubject> iObservers;

    virtual void ResetSubject(SPCSubject& subject)
    {
      subject->MarkRemoveAllListeners();
      subject->MarkRemoveSelfAsListener();
      subject.reset();
    }

    virtual void ProcessMarkRemoveAllListeners(void)
    {
      if (this->iMarkRemoveAllListeners)
      {
        this->RemoveAllEventListenersInternal();        
      }

      for (auto it = iObservers.begin(); it != iObservers.end(); )
      {
        if ((*it)->IsMarkRemoveSelfAsListener())
        {
          it = this->RemoveEventListenerInternal(*it);
        }
        else
        {
          it++;
        }
      }
    }

    virtual void RemoveAllEventListenersInternal()
    {
      iObservers.clear();
      iMarkRemoveAllListeners = false;      
    }

    auto RemoveEventListenerInternal(const SPCSubject& consumer)
    {
      return iObservers.erase(
        std::remove(
          iObservers.begin(), iObservers.end(), consumer),
        iObservers.end()
      );
    }

    virtual void NotifyConnect()
    {
      for (auto& observer : iObservers)
      {
        observer->OnConnect();
      }
      ProcessMarkRemoveAllListeners();
    }

    virtual void NotifyRead(const T2 *b, size_t n)
    {
      for (auto& observer : iObservers)
      {
        observer->OnRead(b, n);
      }
      ProcessMarkRemoveAllListeners();
    }

    virtual void NotifyWrite(const T2 *b, size_t n)
    {
      for (auto& observer : iObservers)
      {
        observer->OnWrite(b, n);
      }
      ProcessMarkRemoveAllListeners();
    }

    virtual void NotifyDisconnect()
    {
      for (auto& observer : iObservers)
      {
        observer->OnDisconnect();
      }
      ProcessMarkRemoveAllListeners();
    }
};

template <typename T1, typename T2>
using SPCSubject = std::shared_ptr<CSubject<T1, T2>>;

template<typename T1, typename T2>
using WPCSubject = std::weak_ptr<CSubject<T1, T2>>;

#endif //COMPONENT_HPP