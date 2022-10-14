# safeboard
This is my solution to the safeboard contest problem.

An algorithm goes as follows:
1. A client application sends a request with a path to scan for suspicious files to a server application. This is done via HTTP get request.
2. An asynchronous server application accepts a connection and scans a desired directory for suspicious files.
3. A response HTTP message based on work of a scanner is formed and sent back to client.
4. Client synchronously waits for a server response and prints it on recieving.

Both client and server applications are made by means of boost.beast library, implementing an HTTP based networking.

## Building
Building requiers gcc compiler and make utility.
Executables are created by following commands:
```
make client
```
and
```
make server
```
## Examples
Client application takes one parameter: directory to inspect. Example:
```
./client /home/arthur/sus_directory
```
Server application does not need any parameters.
## Sources
Client and server sources are located in client.cpp and server.cpp respectively.
Repository also contains scripts for generating and contaminating large amounts of files to test a scanner on. 
