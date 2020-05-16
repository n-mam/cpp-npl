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

  ftp->GetCurrentDir([](const std::string& dir){
    std::cout << "current directory : " << dir << "\n";
  });

  ftp->List([](const std::string& list) {
    std::cout << list << "\n";
  });

  ftp->SetCurrentDir([](const std::string& resp){
    std::cout << resp << "\n";
  }, "xyz");

  ftp->Quit([](const std::string& resp){
    std::cout << resp << "\n";
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(15000));

  return 0;
}
```