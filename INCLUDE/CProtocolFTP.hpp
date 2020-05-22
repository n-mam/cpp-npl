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

namespace NPL {

enum class ESSL : uint8_t
{
  None = 0,
  Implicit,
  Explicit
};

using TUploadCbk = std::function<bool (char **, size_t *)>;
using TDownloadCbk = std::function<bool (char *, size_t)>;
using TRespCbk = std::function<void (const std::string&)>;

class CProtocolFTP : public CProtocol<uint8_t, uint8_t>
{
  public:

    CProtocolFTP()
    {
      iJobInProgress.test_and_set();
    }

    virtual ~CProtocolFTP() {}

    virtual void Upload(TUploadCbk cbk, const std::string& fRemote)
    {
      std::lock_guard<std::mutex> lg(iLock);

      if (!fRemote.size() || !cbk) return;

      iJobQ.emplace_back("PASV", "", "", nullptr, nullptr, nullptr);

      iJobQ.emplace_back("STOR", fRemote, "", nullptr, cbk, nullptr);

      ProcessNextJob();
    }

    virtual void Download(TDownloadCbk cbk, const std::string& fRemote)
    {
      std::lock_guard<std::mutex> lg(iLock);

      if (!fRemote.size() || !cbk) return;

      iJobQ.emplace_back("PASV", "", "", nullptr, nullptr, nullptr);

      iJobQ.emplace_back("RETR", fRemote, "", nullptr, nullptr, cbk);

      ProcessNextJob();
    }

    virtual void List(TDownloadCbk cbk, const std::string& fRemote = "")
    {
      std::lock_guard<std::mutex> lg(iLock);

      if (cbk)
      {
        iJobQ.emplace_back("PASV", "", "", nullptr, nullptr, nullptr);

        iJobQ.emplace_back("LIST", fRemote, "", nullptr, nullptr, cbk);

        ProcessNextJob();        
      }
    }

    virtual void GetCurrentDir(TRespCbk cbk = nullptr)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("PWD", "", "", cbk, nullptr, nullptr);

      ProcessNextJob();
    }

    virtual void SetCurrentDir(const std::string& dir, TRespCbk cbk = nullptr)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("CWD", dir, "", cbk, nullptr, nullptr);

      ProcessNextJob();       
    }

    virtual void CreateDir(const std::string& dir, TRespCbk cbk)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("MKD", dir, "", cbk, nullptr, nullptr);

      ProcessNextJob();    
    }

    virtual void RemoveDir(const std::string& dir, TRespCbk cbk)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("RMD", dir, "", cbk, nullptr, nullptr);

      ProcessNextJob();  
    }

    virtual void Quit(TRespCbk cbk = nullptr)
    {
      std::lock_guard<std::mutex> lg(iLock);

      iJobQ.emplace_back("QUIT", "", "", cbk, nullptr, nullptr);

      ProcessNextJob();       
    }

    virtual void SetFTPSType(ESSL ftps)
    {
      iSSLType = ftps;
    }

  protected:

    ESSL iSSLType = ESSL::None;

    bool iContinueDataCbk = false;

    SPCDevice iFileDevice = nullptr;

    SPCDeviceSocket iDataChannel = nullptr;

    std::atomic_flag iJobInProgress = ATOMIC_FLAG_INIT;

    std::atomic_char iPendingResponse = 0;

    std::list<
      std::tuple<
       std::string,      // command
       std::string,      // fRemote
       std::string,      // fLocal
       TRespCbk,         // rcbk
       TUploadCbk,        // ucbk
       TDownloadCbk      // dcbk       
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
      { "PASS"      , '1', "USER",       [this]() { ProcessLoginEvent(); }          },
      { "PASS"      , '2', "READY" ,     [this]() { ProcessLoginEvent(true); }          },
      { "PASS"      , '3', "ACCT" ,      [this]() { SendCommand("ACCT");}               },
      { "PASS"      , '4', "USER",       [this]() { ProcessLoginEvent(); }          },
      { "PASS"      , '5', "USER",       [this]() { ProcessLoginEvent(); }          },
      // PASV states
      { "PASV"      , '1', "DATA"  ,     [this]() { SkipCommand(2);        }            },      
      { "PASV"      , '2', "DATA"  ,     [this]() { ProcessPasvResponse(); }            },
      { "PASV"      , '4', "READY"  ,    [this]() { SkipCommand(2);        }            },
      { "PASV"      , '5', "READY"  ,    [this]() { SkipCommand(2);        }            },
      // DATA command (LIST RETR STOR) states
      { "DATA"      , '1', "DATA"  ,     [this] () { ProcessDataCmdResponse('1'); }        },
      { "DATA"      , '2', "DATA" ,      [this] () { ProcessDataCmdResponse('2'); }        },
      { "DATA"      , '4', "READY" ,     [this] () { ProcessDataCmdResponse('4'); }        },
      { "DATA"      , '5', "READY" ,     [this] () { ProcessDataCmdResponse('5'); }        },
      { "GEN"       , '1', "READY" ,     [this] () { ProcessGenCmdEvent();  }           },
      { "GEN"       , '2', "READY" ,     [this] () { ProcessGenCmdEvent();  }           },
      { "GEN"       , '3', "READY" ,     [this] () { ProcessGenCmdEvent();  }           },
      { "GEN"       , '4', "READY" ,     [this] () { ProcessGenCmdEvent();  }           },
      { "GEN"       , '5', "READY" ,     [this] () { ProcessGenCmdEvent();  }           }
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
      std::cout << cmd;
      Write((uint8_t *)cmd.c_str(), cmd.size(), 0);
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

        auto& [cmd, fRemote, fLocal, rcbk, ucbk, dcbk] = iJobQ.front();

        iPendingResponse = 1;

        if (IsTransferCommand(cmd))
        {       
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
      auto& [cmd, fRemote, fLocal, rcbk, ucbk, dcbk] = iJobQ.front();

      if (rcbk)
      {
        auto& m = iMessages.back();
        std::string res(m.begin(), m.end());
        rcbk(res);
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

    virtual void ProcessDataCmdResponse(char code = '0')
    {
      auto& [cmd, fRemote, fLocal, rcbk, ucbk, dcbk] = iJobQ.front();

      switch (code)
      {
        case '1':
        {
          iContinueDataCbk = true;

          if (cmd == "STOR")
          {
            if (ucbk)
            {
              uint8_t *b = nullptr;
              size_t n = 0;

              iContinueDataCbk = ucbk((char **)&b, &n);

              if (b && n)
              {
                iDataChannel->Write(b, n);
              }
            }
            else if (iFileDevice)
            {
              iFileDevice->Read(nullptr, 0, 0);
            }
          }
          else
          {
            iContinueDataCbk = true;
          }

          iDataChannel->Read();

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

      if (!iPendingResponse && !iDataChannel->IsConnected())
      {
        ResetDataChannel();

        iJobQ.pop_front();

        iJobInProgress.clear();

        ProcessNextJob();
      }
    }

    virtual void OpenDataChannel(const std::string& host, int port)
    {
      iDataChannel = std::make_shared<CDeviceSocket>();

      iDataChannel->SetName("dc");

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

      observer->SetName("Observer");

      auto D = GetDispatcher();

      D->AddEventListener(iDataChannel)->AddEventListener(observer);

      iDataChannel->SetHostAndPort(host, port);

      iDataChannel->StartSocketClient();
    }

    virtual void OnDataChannelConnect(void)
    {
      auto& [cmd, fRemote, fLocal, rcbk, ucbk, dcbk] = iJobQ.front();

      if (cmd == "STOR" && !ucbk)
      {
        assert(fLocal.size());

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

    virtual void OnDataChannelRead(const uint8_t *b, size_t n)
    {
      auto& [cmd, fRemote, fLocal, rcbk, ucbk, dcbk] = iJobQ.front();

      if (iContinueDataCbk)
      {
        iContinueDataCbk = dcbk((char *)b, n);
      }
      else
      {
        iDataChannel->Shutdown();
      }
    }

    virtual void OnDataChannelWrite(const uint8_t *b, size_t n)
    {
      auto& [cmd, fRemote, fLocal, rcbk, ucbk, dcbk] = iJobQ.front();

      if (iContinueDataCbk)
      {
        iContinueDataCbk = ucbk((char **)&b, &n);

        if (b && n)
        {
          iDataChannel->Write(b, n);
        }
      }
      else
      {
        iDataChannel->Shutdown();
      }
    }

    virtual void OnDataChannelDisconnect(void)
    {
      auto& [cmd, fRemote, fLocal, rcbk, ucbk, dcbk] = iJobQ.front();

      if (dcbk)
      {
        dcbk(nullptr, 0);
      }

      ProcessDataCmdResponse();
    }

    virtual void ProcessLoginEvent(bool status = false)
    {
      iJobInProgress.clear();
      NotifyState("PASS", 'S');
      ProcessNextJob();
    }

    virtual void ResetDataChannel(void)
    {
      iDataChannel->MarkRemoveAllListeners();
      iDataChannel->MarkRemoveSelfAsListener();
      iDataChannel.reset();
    }

    virtual bool IsTransferCommand(const std::string& cmd)
    {
      return (cmd == "RETR" || 
              cmd == "LIST" ||
              cmd == "STOR");
    }

    virtual void OnConnect(void) override
    {
      CProtocol::OnConnect();

      Read();

      auto cc = iTarget.lock();

      if (cc)
      {
        auto sock = std::dynamic_pointer_cast<CDeviceSocket>(cc);

        if (iSSLType == ESSL::Implicit)
        {
          sock->InitializeSSL();
        }
      }
    }
};

using SPCProtocolFTP = std::shared_ptr<CProtocolFTP>;

} //npl namespace
#endif //FTPSEGMENTER_HPP