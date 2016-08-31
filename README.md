# TFTP.ASIO
This TFTP.ASIO is based on boost c++ library. This project implemented a client and server for TFTP. Also it uses boost.asio. 

# How to build
##1. Windows
   TFTP.ASIO has a solution file for Visual Studio 2015, so you can build very easy.
   However, you need to build and install the boost c++ library and to config boost library path in this project before you try to build this project.

##2. Linux
###  Preparation
  You need to build and install the boost c++ library before you try to build this project.

###  Build Steps
```bash
mkdir build
cd build
cmake ..
make
```

# Example - TFTPClient
```C++
#include <iostream>
#include <thread>

#include <boost/asio.hpp>

#include "TFTP.ASIO/AsioTFTPClient.h"

int main(int argc, char* argv[]) {
    std::string params[5];

    if (argc != 6) {
        std::cerr << "Usage: TFTPClient [host] [port] [local_file] [remote_file] [get/put]\n";
        return 0;
    }
    else {
        for (int index = 0; index < 5; index++)
            params[index] = argv[index + 1];
    }

    try {
        boost::asio::io_service io_service;
        boost::asio::io_service::work work(io_service);
        std::thread thread([&io_service]() { io_service.run(); });

        AsioTFTPClient tftp_client(io_service, params[0], params[1]);

        if (params[4] == "put") {
            std::cout << std::endl;
            std::cout << "Now, TFTPClient is going to put a file to TFTP Server." << std::endl << std::endl;

            if (tftp_client.put_file(params[2], params[3]))
                std::cout << " --> Transfer completed." << std::endl;
            else
                std::cout << " --> Transfer failed." << std::endl;
        }
        else {
            std::cout << std::endl;
            std::cout << "Now, TFTPClient is going to get a file from TFTP Server." << std::endl << std::endl;

            if (tftp_client.get_file(params[2], params[3]))
                std::cout << " --> Transfer completed." << std::endl;
            else
                std::cout << " --> Transfer failed." << std::endl;
        }

        tftp_client.bye();

        io_service.stop();
        thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
```

# Example - TFTPServer
```C++
#include <iostream>
#include <thread>

#include <boost/asio.hpp>

#include "TFTP.ASIO/AsioTFTPServer.h"

int main(int argc, char* argv[]) {
    std::string params[2];

    if (argc < 2) {
        std::cerr << "Usage: TFTPServer [local_path] {port}\n";
        return 0;
    }
    else {
        params[0] = argv[1];
        params[1] = "69";

        if (argc > 2) {
            if (argc != 3) {
                std::cerr << "Usage: TFTPServer [local_path] {port}\n";
                return 0;
            }

            params[1] = argv[2];
        }
    }

    try {
        boost::asio::io_service io_service;

        std::cout << std::endl;
        std::cout << "Now, TFTP is ready to service for TFTP Client" << std::endl << std::endl;

        AsioTFTPServer tftp_server(io_service, params[0], params[1]);

        boost::asio::io_service::work work(io_service);
        std::thread thread([&io_service]() { io_service.run(); });

        thread.join();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
```

# Reference Sources
1. TFTP Client and Server Programs in C with Unix Sockets, James Northway: https://jamesnorthway.net/tftp/tftp.html
2. boost.asio: http://www.boost.org/
