#include <iostream>

#include <npl.hpp>

int main(void)
{
  auto ftp = npl::make_ftp("127.0.0.1", 21, "anonymous", "welcome123");

  ftp->GetCurrentDir([](const std::string& resp) {
    std::cout << resp << "\n";
  });

  std::string list;
  ftp->List([&](char *b, size_t n) {
    if (b)
     list.append(std::string(b, n));
    else
     std::cout << list;
  });

  ftp->SetCurrentDir([](const std::string& resp) {
    std::cout << resp << "\n";
  }, "vcpkg");

  // ftp->Upload([](uint8_t *b, size_t n) {
    
  // },"y.txt", "C:\\x.txt");

  ftp->Download([](char *b, size_t n) {
    if (b)
      std::cout << std::string(b, n);
    else
      std::cout << "Download completed\n";
  }, "bootstrap-vcpkg.bat");

  ftp->Quit([](const std::string& resp) {
    std::cout << resp << "\n";
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(15000));

  return 0;
}