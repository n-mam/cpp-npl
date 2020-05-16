#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <Common.hpp>
#include <CSubject.hpp>

#include <memory>
#include <assert.h>
#include <inttypes.h>

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
    uint8_t *      b;
    unsigned long  n;
};

class CDevice : public CSubject<uint8_t, uint8_t>
{
  public:

    FD iFD;

    CDevice() = default;

    virtual ~CDevice() {};

    virtual void Read(uint8_t *b = nullptr, size_t l = 0, uint64_t o = 0) override
    {
      if (iConnected)
      {
        Context *ctx = (Context *) calloc(1, sizeof(Context));

        ctx->type = EIOTYPE::READ;

        if (b)
        {
          ctx->b = b;
        }
        else
        {
          ctx->b = (uint8_t *) calloc(1, 10);
          l = 10;
        }

        #ifdef linux

        #else

        (ctx->ol).Offset = o & 0x00000000FFFFFFFF;
        (ctx->ol).OffsetHigh = (o & 0xFFFFFFFF00000000) >> 32;

        ReadFile(iFD, (LPVOID) ctx->b, l, &ctx->n, &ctx->ol);

        #endif
      }
    }

    virtual void Write(uint8_t *b = nullptr, size_t l = 0, uint64_t o = 0) override
    {
      if (iConnected)
      {
        if (b && l)
      {
        Context *ctx = (Context *) calloc(1, sizeof(Context));

        ctx->type = EIOTYPE::WRITE;

        ctx->b = b;

        #ifdef linux

        #else

        (ctx->ol).Offset = o & 0x00000000FFFFFFFF;
        (ctx->ol).OffsetHigh = (o & 0xFFFFFFFF00000000) >> 32;

        for (int i = 0; i < l; i++)
        {
          std::cout << b[i];
        }

        WriteFile(iFD, (LPVOID) b, l, &ctx->n, &ctx->ol);

        #endif
      }
    }
    }

  protected:

};

using SPCDevice = std::shared_ptr<CDevice>;

#endif //DEVICE_HPP