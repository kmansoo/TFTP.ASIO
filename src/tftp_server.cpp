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
        std::cout << "Now, TFTPServer is ready to service for Client" << std::endl << std::endl;

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
