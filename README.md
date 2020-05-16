# cpp-np-lib

C++17 network protocol library

- Cross platform
- Header only
- Supported protocols
    * FTP
	* FTPS
	* SSH
	* Websockets

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

  Sleep(10000);

  return 0;
}
```