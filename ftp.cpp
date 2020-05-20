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
    return true;
  });

  ftp->SetCurrentDir("vcpkg");

  ftp->Upload([](char **b, size_t *n) {
    *b = "This is file data.";
    *n = strlen("This is file data.");
    return false;
  }, "y.txt");

  ftp->Download([](char *b, size_t n) {
    if (b)
      std::cout << std::string(b, n);
    else
      std::cout << "Download completed\n";
    return true;
  }, "bootstrap-vcpkg.bat");

  ftp->Quit();

  std::this_thread::sleep_for(std::chrono::milliseconds(15000));

  return 0;
}