### cpp-np-lib

C++17 network protocol library

- Cross platform
- Header only
- Asynchronous
- Supported protocols
  * FTP
  * FTPS (WIP)
  * SSH (WIP)
  * Websockets (WIP)

Build

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

FTP example
```cpp
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

  ftp->SetCurrentDir([](const std::string& resp) {
    std::cout << resp << "\n";
  }, "vcpkg");

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

  ftp->Quit([](const std::string& resp) {
    std::cout << resp << "\n";
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(15000));

  return 0;
}
```