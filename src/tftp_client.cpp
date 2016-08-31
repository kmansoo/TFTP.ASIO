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
