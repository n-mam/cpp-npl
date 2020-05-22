#include <iostream>

#include <npl.hpp>

int main(void)
{
  /**
   * Create an FTP object.
   */
  auto ftp = NPL::make_ftp("127.0.0.1", 990, NPL::ESSL::Implicit);

  /**
   * Set the login credentials.
   */
  ftp->SetCredentials("anonymous", "welcome123");

  /**
   * Start the protocol(FTP). This would asynchronously
   * trigger the connect and login sequence.
   */
  ftp->StartProtocol();

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