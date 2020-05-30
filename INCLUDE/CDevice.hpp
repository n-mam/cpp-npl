#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <Common.hpp>
#include <CSubject.hpp>

#include <memory>
#include <assert.h>
#include <inttypes.h>

namespace NPL {

enum EIOTYPE : uint8_t
{
  INIT = 0,
  READ = 1,
  WRITE = 2,
  IOCTL = 3,
};

struct Context
{
  #ifdef linux

  #else
    OVERLAPPED     ol;
  #endif
    EIOTYPE        type;
    const uint8_t *b;
    unsigned long  n;
};

constexpr uint32_t DEVICE_BUFFER_SIZE = 10;

class CDevice : public CSubject<uint8_t, uint8_t>
{
  public:

    FD iFD;

    CDevice() = default;

    CDevice(const std::string& aFilename, bool bCreateNew)
    {
      #ifdef WIN32
        iFD = CreateFileA(
          aFilename.c_str(),
          GENERIC_READ|GENERIC_WRITE,
          FILE_SHARE_READ|FILE_SHARE_WRITE,
          NULL,
          (bCreateNew ? CREATE_ALWAYS : OPEN_ALWAYS),
          FILE_FLAG_OVERLAPPED|FILE_FLAG_SEQUENTIAL_SCAN,
          NULL);

        if (iFD != INVALID_HANDLE_VALUE)
        {
          iConnected = true;
        }
        else
        {
          std::cout << GetLastError();
        }
      #endif 
    }

    virtual ~CDevice() {};

    virtual void * Read(const uint8_t *b = nullptr, size_t l = 0, uint64_t o = 0) override
    {
      if (!iConnected)
      {
        std::cout << "CDevice::Read() Not connected\n";
        return nullptr;
      }

      #ifdef linux
      if (!b) return nullptr;
      #endif

      Context *ctx = (Context *) calloc(1, sizeof(Context));

      ctx->type = EIOTYPE::READ;

      if (b)
      {
        ctx->b = b;
      }
      else
      {
        ctx->b = (uint8_t *) calloc(1, DEVICE_BUFFER_SIZE);
        l = DEVICE_BUFFER_SIZE;
      }

      #ifdef linux

      ctx->n = read(iFD, (void *) ctx->b, l);

      return (void *) ctx;

      #else

      (ctx->ol).Offset = o & 0x00000000FFFFFFFF;
      (ctx->ol).OffsetHigh = (o & 0xFFFFFFFF00000000) >> 32;

      BOOL fRet = ReadFile(iFD, (LPVOID) ctx->b, l, NULL, &ctx->ol);

      if (!fRet && GetLastError() != ERROR_IO_PENDING)
      {
        std::cout << "WriteFile failed : " << GetLastError() << "\n";
      }

      return nullptr;

      #endif
    }

    virtual int64_t Write(const uint8_t *b = nullptr , size_t l = 0, uint64_t o = 0) override
    {
      if (!iConnected)
      {
        std::cout << "CDevice::Write() Not connected\n";
        return -1;
      }

      assert(b && l);

      Context *ctx = (Context *) calloc(1, sizeof(Context));

      ctx->type = EIOTYPE::WRITE;

      ctx->b = b;

      #ifdef linux

      return write(iFD, b, l);

      #else

      (ctx->ol).Offset = o & 0x00000000FFFFFFFF;
      (ctx->ol).OffsetHigh = (o & 0xFFFFFFFF00000000) >> 32;

      BOOL fRet = WriteFile(iFD, (LPVOID) b, l, NULL, &ctx->ol);

      if (!fRet && GetLastError() != ERROR_IO_PENDING)
      {
        std::cout << "WriteFile failed : " << GetLastError() << "\n";
      }

      return -1;

      #endif
    }

  protected:

};

using SPCDevice = std::shared_ptr<CDevice>;
using WPCDevice = std::weak_ptr<CDevice>;

} //namespace NPL

#endif //DEVICE_HPP