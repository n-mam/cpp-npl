#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <functional>

namespace NPL
{
  using TLogCallback = std::function<void (const std::string&)>;

  class CLogger
  {
    public:

    CLogger(){}

    ~CLogger()
    {
      if (m_instance) 
      {
        delete m_instance;
      }
    }

    static CLogger& Log()
    {
      if(!m_instance)
      {
        m_instance = new CLogger();
      }

      return *m_instance;
    }

    void operator << (const std::string& line)
    {
      if (m_cbk)
      {
        m_cbk(line);
      }
      else
      {
        std::cout << line <<std::endl;
      }
    }

    static void SetLogCallback(TLogCallback cbk)
    {
      m_cbk = cbk;
    }

    protected:

    static CLogger * m_instance;
    static TLogCallback m_cbk;
  };

  TLogCallback CLogger::m_cbk = nullptr;
  CLogger * CLogger::m_instance = nullptr;
}

#define LOG NPL::CLogger::Log()

#endif