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
Install openssl using vcpkg
https://github.com/Microsoft/vcpkg
```
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
  /**
   * Create "ftp" object. This would asynchronously
   * trigger the connect and login sequence.
   */
  auto ftp = npl::make_ftp("127.0.0.1", 21, "anonymous", "welcome123");

  /**
   * Directory listing. The lambda argument is an output callback 
   * which is invoked multiple times with chunks of directory list
   * data. A null "b" (buffer) pointer indicates that there's no more data.
   */
  std::string list;
  ftp->List([&](char *b, size_t n) {
    if (b)
     list.append(std::string(b, n));
    else
     std::cout << list;
    return true;
  });

  /**
   * Set the current directory
   */
  ftp->SetCurrentDir("vcpkg");

  /**
   * File download. Similar to the "List" API. 
   * The lambda argument is an output callback which is invoked multiple times 
   * with chunks of directory list data. A null "b" (buffer) pointer indicates 
   * that there's no more data.
   */
  ftp->Download([](char *b, size_t n) {
    if (b)
      std::cout << std::string(b, n);
    else
      std::cout << "Download completed\n";
    return true;
  }, "bootstrap-vcpkg.bat");

  /**
   * File upload. The lambda argument is an input callback which is invoked
   * multiple times with chunks of file data. Returning false from this
   * callback indicates that there is no more file data to be uploded. Return
   * true for recieving subsequent callbacks
   */
  ftp->Upload([](char **b, size_t *n) {
    *b = "This is file data.";
    *n = strlen("This is file data.");
    return false;
  }, "y.txt");

  /**
   * Get the current directory
   */
  ftp->GetCurrentDir([](const std::string& resp) {
    /**
     * "resp" holds the response of PWD command
     */
  });

  /**
   * Quit the session. Sends FTP QUIT command
   * and triggeres the cleanup of "ftp" object
   */
  ftp->Quit();
 
  std::this_thread::sleep_for(std::chrono::milliseconds(15000));

  return 0;
}
```