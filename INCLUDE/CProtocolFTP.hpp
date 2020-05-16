#ifndef FTPSEGMENTER_HPP
#define FTPSEGMENTER_HPP

#include <CProtocol.hpp>
#include <CListener.hpp>

#include <list>
#include <tuple>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <functional>

using TResponseCbk = std::function<void (const std::string&)>;

class CProtocolFTP : public CProtocol<uint8_t, uint8_t>
{
  public:

    CProtocolFTP()
    {
      iJobInProgress.test_and_set();
    }

    virtual ~CProtocolFTP() {}

    virtual void Upload(const std::string& fRemote, const std::string& fLocal)
    {
      std::lock_guard<std::recursive_mutex> lg(iLock);

      iJobQ.emplace_back("PASV", "", "", nullptr);

      iJobQ.emplace_back("STOR", fRemote, fLocal, nullptr);

      ProcessNextJob();
    }

    virtual void Download(TResponseCbk cbk, const std::string& fRemote, const std::string& fLocal = "")
    {
      std::lock_guard<std::recursive_mutex> lg(iLock);

      iJobQ.emplace_back("PASV", "", "", nullptr);

      iJobQ.emplace_back("RETR", fRemote, fLocal, cbk);

      ProcessNextJob();
    }

    virtual void List(TResponseCbk cbk, const std::string& fRemote = "")
    {
      std::lock_guard<std::recursive_mutex> lg(iLock);

      if (cbk)
      {
        iJobQ.emplace_back("PASV", "", "", nullptr);

        iJobQ.emplace_back("LIST", fRemote, "", cbk);

        ProcessNextJob();        
      }
    }

    virtual void GetCurrentDir(TResponseCbk cbk)
    {
      std::lock_guard<std::recursive_mutex> lg(iLock);

      iJobQ.emplace_back("PWD", "", "", cbk);

      ProcessNextJob();       
    }

    virtual void SetCurrentDir(TResponseCbk cbk, const std::string& dir)
    {
      std::lock_guard<std::recursive_mutex> lg(iLock);

      iJobQ.emplace_back("CWD", dir, "", cbk);

      ProcessNextJob();       
    }

    virtual void CreateDir(TResponseCbk cbk, const std::string& dir)
    {
      std::lock_guard<std::recursive_mutex> lg(iLock);

      iJobQ.emplace_back("MKD", dir, "", cbk);

      ProcessNextJob();    
    }

    virtual void RemoveDir(TResponseCbk cbk, const std::string& dir)
    {
      std::lock_guard<std::recursive_mutex> lg(iLock);

      iJobQ.emplace_back("RMD", dir, "", cbk);

      ProcessNextJob();  
    }

    virtual void Quit(TResponseCbk cbk)
    {
      std::lock_guard<std::recursive_mutex> lg(iLock);

      iJobQ.emplace_back("QUIT", "", "", cbk);

      ProcessNextJob();       
    }

  protected:

    std::string iDirectoryList;

    std::atomic_flag iJobInProgress = ATOMIC_FLAG_INIT;

    std::atomic_char iPendingReplies = 0;

    SPCDeviceSocket iDataChannel = nullptr;

    std::list<
      std::tuple<
       std::string,      // command
       std::string,      // fRemote
       std::string,      // fLocal
       TResponseCbk      // cbk
    >> iJobQ;

    using TStateFn = std::function<void (void)>;

    struct Transition
    {
      std::string  iTransitionState;
      char         iResponseCode;
      std::string  iNextState;
      TStateFn     iTransitionFn;
    };

    Transition FSM[25] =
    {
      // Connection states
      { "CONNECTED" , '1', "CONNECTED" , nullptr                                        },
      { "CONNECTED" , '2', "USER"      , [this](){ SendCommand("USER", iUserName); }    },
      { "CONNECTED" , '4', "CONNECTED" , nullptr                                        },
      // USER states
      { "USER"      , '1', "USER",       [this](){ }                                    },
      { "USER"      , '2', "READY",      [this](){ }                                    },
      { "USER"      , '3', "PASS",       [this](){ SendCommand("PASS", iPassword);}     },
      { "USER"      , '4', "USER",       [this](){ }                                    },
      { "USER"      , '5', "USER",       [this](){ }                                    },
      // PASS states
      { "PASS"      , '1', "USER",       [this]() { NotifyState("PASS", 'E');}          },
      { "PASS"      , '2', "READY" ,     [this]() { OnLogin(); }          },
      { "PASS"      , '3', "ACCT" ,      [this]() { SendCommand("ACCT");}               },
      { "PASS"      , '4', "USER",       [this]() { NotifyState("PASS", 'F');}          },
      { "PASS"      , '5', "USER",       [this]() { NotifyState("PASS", 'F');}          },
      // PASV states
      { "PASV"      , '2', "DATA"  ,     [this]() { ProcessPasvResponse('2'); }            },
      { "PASV"      , '4', "READY"  ,    [this]() { ProcessPasvResponse('4'); }            },
      { "PASV"      , '5', "READY"  ,    [this]() { ProcessPasvResponse('5'); }            },
      // DATA command (LIST RETR STOR) states
      { "DATA"      , '1', "DATA"  ,     [this] () { ProcessDataCmdEvent('1'); }        },
      { "DATA"      , '2', "DATA" ,      [this] () { ProcessDataCmdEvent('2'); }        },
      { "DATA"      , '4', "READY" ,     [this] () { ProcessDataCmdEvent('4'); }        },
      { "DATA"      , '5', "READY" ,     [this] () { ProcessDataCmdEvent('5'); }        },
      { "GEN"       , '1', "READY" ,     [this] () { ProcessGenCmdEvent('1');  }        },
      { "GEN"       , '2', "READY" ,     [this] () { ProcessGenCmdEvent('2');  }        },
      { "GEN"       , '3', "READY" ,     [this] () { ProcessGenCmdEvent('3');  }        },
      { "GEN"       , '4', "READY" ,     [this] () { ProcessGenCmdEvent('4');  }        },
      { "GEN"       , '5', "READY" ,     [this] () { ProcessGenCmdEvent('5');  }        }
    };

    virtual void StateMachine(const std::vector<uint8_t>& msg) override
    {
      for (size_t i = 0; i < msg.size(); i++)
      {
        std::cout << msg[i];
      }

      std::cout << "\n";

      for (int i = 0; i < sizeof(FSM) / sizeof(FSM[0]); i++)
      {
        Transition t = FSM[i];

        if ((iProtocolState == t.iTransitionState) && (t.iResponseCode == msg[0]))
        {
          iProtocolState = t.iNextState;

          if (t.iTransitionFn)
          {
            t.iTransitionFn();
          }

          break;
        }
      }      
    }

    virtual bool IsMessageComplete(const std::vector<uint8_t>& b) override
    {
      size_t l = b.size();

      if (l >= 4)
      {
        if ((b[l-2] == '\r') && (b[l-1] == '\n'))
        {
          uint8_t code[4] = { 0, 0, 0, ' '};

          memmove(code, b.data(), 3);

          for (size_t i = 0; i < l; i++)
          {
            if (0 == memcmp(b.data() + i, code, 4))
            {
              return true;
            }
          }
        }
      }
      return false;
    }

    virtual void SendCommand(const std::string& c, const std::string& arg = "")
    {
      auto cmd = c + " " + arg + "\r\n";
      Write((uint8_t *)cmd.c_str(), cmd.size(), 0);
    }

    virtual void OnLogin(void)
    {
      iJobInProgress.clear();
      NotifyState("PASS", 'S');
      ProcessNextJob();
    }

    virtual bool IsGenericCommand(const std::string& cmd)
    {
      return (
        cmd != "RETR" && 
        cmd != "LIST" && 
        cmd != "STOR" && 
        cmd != "PASV" &&
        cmd != "PORT");
    }

    virtual void ProcessNextJob(void)
    {
      if (iJobInProgress.test_and_set() == false)
      {
        assert(iPendingReplies.load() == 0);

        assert(iProtocolState == "READY");

        if (!iJobQ.size())
        {
          iJobInProgress.clear();
          return;
        }

        auto& [command, fRemote, fLocal, listcbk] = iJobQ.front();

        iPendingReplies = 1;

        iProtocolState = IsGenericCommand(command) ? "GEN": command;

        SendCommand(command, fRemote);
      }
    }

    virtual void ProcessGenCmdEvent(char code)
    {
      auto& [command, fRemote, fLocal, listcbk] = iJobQ.front();

      if (listcbk)
      {
        auto& m = iMessages.back();
        std::string response(m.begin(), m.end());
        listcbk(response);
      }

      iPendingReplies--;
      iJobQ.pop_front();
      iJobInProgress.clear();
      ProcessNextJob();
    }

    virtual void ProcessPasvResponse(char code)
    {
      if (code == '2')
      {
        auto& m = iMessages.back();

        std::string pasv(m.begin(), m.end());

        size_t first = pasv.find("(") + 1;
        size_t last = pasv.find(")");

        auto spec = pasv.substr(first, last - first);

        uint32_t h1,h2,h3,h4,p1,p2;
        int fRet = sscanf(spec.c_str(), "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);

        if (fRet < 6)
        {
          std::cout << "Faled to parse PASV response\n";
        }

        auto host = std::to_string(h1) + "." + 
                    std::to_string(h2) + "." +
                    std::to_string(h3) + "." +
                    std::to_string(h4);

        int port = (p1 << 8) + p2;

        iJobQ.pop_front();

        iPendingReplies--;

        OpenDataChannel(host, port);
      }
      else
      {
        iJobQ.pop_front(); 
        iJobQ.pop_front();
        iPendingReplies = 0;
        iJobInProgress.clear();
      }
    }

    virtual void ProcessDataCmdEvent(char code = '0')
    {
      switch (code)
      {
        case '2':
        case '4':
        case '5':
        {
          iPendingReplies--;
          break;
        }
      }

      if (!iPendingReplies && !iDataChannel->IsConnected())
      {
        iDataChannel->RemoveAllEventListeners();

        auto D = GetDispatcher();

        D->RemoveEventListener(iDataChannel);

        assert(iDataChannel.use_count() == 1);

        iJobQ.pop_front();

        iProtocolState = "READY";

        iJobInProgress.clear();

        if (iJobQ.size())
        {
          ProcessNextJob();
        }
      }
    }

    virtual void OpenDataChannel(const std::string& host, int port)
    {
      assert(iProtocolState == "DATA");

      iDataChannel.reset();

      iDataChannel = std::make_shared<CDeviceSocket>();

      auto observer = std::make_shared<CListener>(
        [this]() {
          DataChannelConnect();
        },
        [this](const uint8_t *b, size_t n) {
          DataChannelRead(b, n);
        },
        [this](const uint8_t *b, size_t n) {
          DataChannelWrite(b, n);
        },
        [this](){
          DataChannelDisconnect();
        });

      auto D = GetDispatcher();

      D->AddEventListener(iDataChannel)->AddEventListener(observer);

      iDataChannel->StartSocketClient(host, port);
    }

    virtual void DataChannelConnect(void)
    {
      assert (!iJobQ.empty());

      auto& [command, fRemote, fLocal, listcbk] = iJobQ.front();

      assert(command == "LIST" || command == "RETR" || command == "STOR");

      iPendingReplies = 1;

      SendCommand(command, fRemote.c_str());
    }

    virtual void DataChannelRead(const uint8_t *b, size_t n)
    {
      auto& [command, fRemote, fLocal, listcbk] = iJobQ.front();

      if (command == "LIST")
      {
        iDirectoryList.append((char *)b, n);
      }
      else
      {
        std::cout << std::string((char *)b, n);
      }
    }

    virtual void DataChannelWrite(const uint8_t *b, size_t n)
    {
      auto& [command, fRemote, fLocal, listcbk] = iJobQ.front();
    }

    virtual void DataChannelDisconnect(void)
    {
      auto& [command, fRemote, fLocal, listcbk] = iJobQ.front();

      if (command == "LIST" && listcbk)
      {
        listcbk(iDirectoryList);
        iDirectoryList = "";
      }

      ProcessDataCmdEvent();
    }
};

using SPCProtocolFTP = std::shared_ptr<CProtocolFTP>;

#endif //FTPSEGMENTER_HPP