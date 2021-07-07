#include <npl.hpp>
#include <osl.hpp>

#include <memory>
#include <iostream>

constexpr int BUFSIZE = (1 * 1024 * 1024);

int main(int argc, char* argv[])
{
  auto arguments = OSL::GetArgumentsVector(argc, argv);

  auto rd = NPL::make_file(arguments[0]);
  rd->SetProperty("name", "rd");

  auto wd = NPL::make_file(arguments[1], true);
  wd->SetProperty("name", "wd");

  uint8_t *buf = (uint8_t *) calloc(BUFSIZE, 1);

  auto obv = std::make_shared<NPL::CListener>(
    nullptr,
    [wd, off = 0ULL](const uint8_t *b, size_t n) mutable
    {
      std::cout << "onread " << n << ", off " << off << "\n";
      if (n)
      {
        wd->Write(b, n, off);
      }
      off += n;
    },
    [rd, off = 0ULL](const uint8_t *b, size_t n) mutable
    {
      std::cout << "onwrite " << n << ", off " << off << "\n";
      if (n)
      {
        rd->Read(b, BUFSIZE, off += n);
      }
    });

  obv->SetProperty("name", "observer");

  rd->AddEventListener(obv);
  wd->AddEventListener(obv);

  rd->Read(buf, BUFSIZE, 0);

  getchar();

  return 0;
}