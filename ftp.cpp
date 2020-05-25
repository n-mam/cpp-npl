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
   * Start the protocol. This would asynchronously
   * trigger connect and the login sequence
   */
  ftp->StartProtocol();

  /**
   * Directory listing. The lambda argument is an output callback 
   * which is invoked multiple times with chunks of directory list
   * data. A null "b" (buffer) pointer indicates that there's no 
   * more data. Returning false at any point terminates the transfer.
   */
  std::string list;
  ftp->List([&](const char *b, size_t n) {
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
   * The lambda argument is an output callback which is invoked multiple 
   * times with chunks of file data. A null "b" (buffer) pointer indicates 
   * that there's no more file data to download. Returning false at any point
   * terminates the transfer. local file arg is optional; if not specified then 
   * downloded data can only be accesed via the callback.
   */
  ftp->Download([](const char *b, size_t n) {
    if (b)
      std::cout << std::string(b, n);
    else
      std::cout << "Download completed\n";
    return true;
  }, "bootstrap-vcpkg.bat", "c:\\download.bat", NPL::EDCProt::Protected);

  /**
   * File upload. The lambda argument is an output callback which is invoked
   * multiple times with chunks of file data as they are uploaded. A null "b"
   * (buffer) pointer indicates that there's no more file data to be uploaded.
   * Returning false from this callback closes the transfer. Data channel 
   * protection is 'C'lear for the upload implying a granular PROT levels on a 
   * per transfer basis.
   */
  ftp->Upload([](const char *b, size_t n) {
    return true;
  }, "y.txt", "C:\\x.txt", NPL::EDCProt::Clear);

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
 
  std::this_thread::sleep_for(std::chrono::milliseconds(8000));

  return 0;
}