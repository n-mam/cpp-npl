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
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("PASV", "", "", nullptr);

      iJobQ.emplace_back("STOR", fRemote, fLocal, nullptr);

      ProcessNextJob();
    }

    virtual void Download(TResponseCbk cbk, const std::string& fRemote, const std::string& fLocal = "")
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("PASV", "", "", nullptr);

      iJobQ.emplace_back("RETR", fRemote, fLocal, cbk);

      ProcessNextJob();
    }

    virtual void List(TResponseCbk cbk, const std::string& fRemote = "")
    {
      std::lock_guard<std::mutex> lg(iLock);

      if (cbk)
      {
        iJobQ.emplace_back("PASV", "", "", nullptr);

        iJobQ.emplace_back("LIST", fRemote, "", cbk);

        ProcessNextJob();        
      }
    }

    virtual void GetCurrentDir(TResponseCbk cbk)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("PWD", "", "", cbk);

      ProcessNextJob();       
    }

    virtual void SetCurrentDir(TResponseCbk cbk, const std::string& dir)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("CWD", dir, "", cbk);

      ProcessNextJob();       
    }

    virtual void CreateDir(TResponseCbk cbk, const std::string& dir)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("MKD", dir, "", cbk);

      ProcessNextJob();    
    }

    virtual void RemoveDir(TResponseCbk cbk, const std::string& dir)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("RMD", dir, "", cbk);

      ProcessNextJob();  
    }

    virtual void Quit(TResponseCbk cbk)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("QUIT", "", "", cbk);

      ProcessNextJob();       
    }

  protected:

    std::string iDirectoryList;

    SPCDevice iFileDevice = nullptr;

    SPCDeviceSocket iDataChannel = nullptr;

    std::atomic_flag iJobInProgress = ATOMIC_FLAG_INIT;

    std::atomic_char iPendingResponse = 0;

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

    Transition FSM[26] =
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
      { "PASV"      , '1', "DATA"  ,     [this]() { SkipCommand(2);        }            },      
      { "PASV"      , '2', "DATA"  ,     [this]() { ProcessPasvResponse(); }            },
      { "PASV"      , '4', "READY"  ,    [this]() { SkipCommand(2);        }            },
      { "PASV"      , '5', "READY"  ,    [this]() { SkipCommand(2);        }            },
      // DATA command (LIST RETR STOR) states
      { "DATA"      , '1', "DATA"  ,     [this] () { ProcessDataCmdEvent('1'); }        },
      { "DATA"      , '2', "DATA" ,      [this] () { ProcessDataCmdEvent('2'); }        },
      { "DATA"      , '4', "READY" ,     [this] () { ProcessDataCmdEvent('4'); }        },
      { "DATA"      , '5', "READY" ,     [this] () { ProcessDataCmdEvent('5'); }        },
      { "GEN"       , '1', "READY" ,     [this] () { ProcessGenCmdEvent();  }           },
      { "GEN"       , '2', "READY" ,     [this] () { ProcessGenCmdEvent();  }           },
      { "GEN"       , '3', "READY" ,     [this] () { ProcessGenCmdEvent();  }           },
      { "GEN"       , '4', "READY" ,     [this] () { ProcessGenCmdEvent();  }           },
      { "GEN"       , '5', "READY" ,     [this] () { ProcessGenCmdEvent();  }           }
    };

    virtual void StateMachine(const std::vector<uint8_t>& msg) override
    {
      std::cout << "\n";

      for (size_t i = 0; i < msg.size(); i++)
      {
        std::cout << msg[i];
      }

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

    virtual bool IsTransferCommand(const std::string& cmd)
    {
      return (
        cmd == "RETR" || 
        cmd == "LIST" ||
        cmd == "STOR");
    }
    
    virtual void SkipCommand(int count = 1)
    {
      for (int i = 0; i < count; i++)
      {
        iJobQ.pop_front();
        iPendingResponse--;
      }

      assert(iPendingResponse.load() == 0);

      iJobInProgress.clear();

      ProcessNextJob();
    }

    virtual void ProcessNextJob(void)
    {
      if (iJobInProgress.test_and_set() == false)
      {
        assert(iPendingResponse.load() == 0);

        if (!iJobQ.size())
        {
          iJobInProgress.clear();
          return;
        }

        auto& [cmd, fRemote, fLocal, cbk] = iJobQ.front();

        iPendingResponse = 1;

        if (IsTransferCommand(cmd))
        {
          iPendingResponse = 2;          
          iProtocolState = "DATA";
        }
        else if (cmd == "PASV")
        {
          iProtocolState = "PASV";
        }
        else
        {
          iProtocolState = "GEN";
        }

        SendCommand(cmd, fRemote);
      }
    }

    virtual void ProcessGenCmdEvent(void)
    {
      auto& [cmd, fRemote, fLocal, cbk] = iJobQ.front();

      if (cbk)
      {
        auto& m = iMessages.back();
        std::string resp(m.begin(), m.end());
        cbk(resp);
      }

      SkipCommand(1);
    }

    virtual void ProcessPasvResponse(void)
    {
      auto& m = iMessages.back();

      std::string pasv(m.begin(), m.end());

      auto spec = pasv.substr(pasv.find('('));

      uint32_t h1, h2, h3, h4, p1, p2;

      int fRet = sscanf(spec.c_str(), "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);

      if (fRet < 6)
      {
        std::cout << "Faled to parse PASV response\n";
      }

      auto host = std::to_string(h1) + "." + 
                  std::to_string(h2) + "." +
                  std::to_string(h3) + "." +
                  std::to_string(h4);

      int port = (p1 << 8) + p2;

      SkipCommand(1);

      OpenDataChannel(host, port);
    }

    virtual void ProcessDataCmdEvent(char code = '0')
    {
      switch (code)
      {
        case '1':
        {
          if (iFileDevice)
          {
            iFileDevice->Read(nullptr, 0, 0);
          }
          iPendingResponse--;
          break;
        }
        case '2':
        case '4':
        case '5':
        {
          iPendingResponse--;
          break;
        }
      }
      // For downloads IsConnected would fail
      // For uploads iDataChannel would be null as we reset it
      if (!iPendingResponse && (!iDataChannel || !iDataChannel->IsConnected()))
      {
        if (iDataChannel)
        {
          ResetDataChannel();
        }

        iJobQ.pop_front();

        iJobInProgress.clear();

        ProcessNextJob();
      }
    }

    virtual void OpenDataChannel(const std::string& host, int port)
    {
      iDataChannel = std::make_shared<CDeviceSocket>();

      auto observer = std::make_shared<CListener>(
        [this]() {
          OnDataChannelConnect();
        },
        [this](const uint8_t *b, size_t n) {
          OnDataChannelRead(b, n);
        },
        [this](const uint8_t *b, size_t n) {
          OnDataChannelWrite(b, n);
        },
        [this](){
          OnDataChannelDisconnect();
        });

      auto D = GetDispatcher();

      D->AddEventListener(iDataChannel)->AddEventListener(observer);

      iDataChannel->StartSocketClient(host, port);
    }

    virtual void OnDataChannelConnect(void)
    {
      auto& [cmd, fRemote, fLocal, cbk] = iJobQ.front();

      if (cmd == "STOR")
      {
        iFileDevice = std::make_shared<CDevice>(fLocal.c_str());

        auto observer = std::make_shared<CListener>(
          nullptr,
          [this, offset = 10] (const uint8_t *b, size_t n) mutable {
            iDataChannel->Write(b, n);
            iFileDevice->Read(nullptr, 0, offset);
            offset += 10;
          },
          nullptr,
          [this](){
            iFileDevice->MarkRemoveAllListeners();
            iFileDevice->MarkRemoveSelfAsListener();
            iDataChannel->Shutdown();
          }
        );

        auto D = GetDispatcher();

        D->AddEventListener(iFileDevice)->AddEventListener(observer);
      }
    }

    virtual void ResetDataChannel(void)
    {
      iDataChannel->MarkRemoveAllListeners();
      iDataChannel->MarkRemoveSelfAsListener();
      iDataChannel.reset();
    }

    virtual void OnDataChannelRead(const uint8_t *b, size_t n)
    {
      auto& [cmd, fRemote, fLocal, cbk] = iJobQ.front();

      if (cmd == "LIST")
      {
        iDirectoryList.append((char *)b, n);
      }
      else if (cmd == "RETR")
      {
        if (cbk)
        {
          cbk(std::string((char *)b, n));
        }
      }
    }

    virtual void OnDataChannelWrite(const uint8_t *b, size_t n)
    {
      auto& [command, fRemote, fLocal, listcbk] = iJobQ.front();
    }

    virtual void OnDataChannelDisconnect(void)
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