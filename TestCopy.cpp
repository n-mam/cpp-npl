#include <iostream>
#include <memory>

#include <npl.hpp>
#include <osl.hpp>

constexpr int BUFSIZE = (1 * 1024 * 1024);

int main(int argc, char* argv[])
{
  auto rd = NPL::make_file(argv[1]);
  rd->SetName("rd");

  auto wd = NPL::make_file(argv[2], true);
  wd->SetName("wd");

  uint8_t *buf = (uint8_t *) calloc(BUFSIZE, 1);

  auto ob = std::make_shared<NPL::CListener>(
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

  ob->SetName("ob");

  rd->AddEventListener(ob);
  wd->AddEventListener(ob);

  rd->Read(buf, BUFSIZE, 0);

  getchar();

  return 0;
}

// onread 1048576, off 267386880
// onwrite 1048576, off 267386880
// onread 1048576, off 268435456
// onwrite 1048576, off 268435456
// onread 560640, off 269484032
// onwrite 560640, off 269484032
// rd ReadFile failed : 998

// onread 364, off 0
// onwrite 364, off 0
// 000001D31BB03190 GQCS failed : 38