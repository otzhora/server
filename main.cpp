#include <iostream>
#include "chat_message.h"
#include "func.h"
#include "chat.h"

typedef boost::shared_ptr<chat_server> chat_server_ptr;

int main(int argc, char** argv) {

    try
    {
        int port;
        if (argc < 2)
        {
            std::cerr << "Usage: chat_server <port> [<port> ...]\n";
            port = 8000;
        }
        else
        {
            port = atoi(argv[1]);
        }

        boost::asio::io_service io_service;

        using namespace std;
        tcp::endpoint ep(tcp::v4(), port);
        chat_server_ptr server(new chat_server(io_service, ep));

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;

    return 0;
}