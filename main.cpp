#include <iostream>

#include <memory>       // std::unique_ptr
#include <algorithm>   // std::find

#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

#include "Socket.hpp"

using namespace std;
using namespace jelford;

void forward(Socket& incoming, Socket& destination, bool& round_over)
{
    auto data = incoming.read();

    wait_for_write(&destination);
    destination.write(data);

    if (data.size() == 0)
        // Other end has hung up
        round_over = true;

    auto tmp = data;
    tmp.push_back('\0');
    cerr << incoming.identify() << "->" << destination.identify() << ": " << &tmp[0] << endl;
}

int main()
{
    // Address/port descriptions for incoming connection
    in_addr_t listen_addr = htonl((((127 << 8) | 0) << 8 | 0) << 8 | 1);
    in_port_t listen_port = htons(5955);

    sockaddr_in listen_address;
    memset(&listen_address, 0, sizeof(listen_address));

    listen_address.sin_family = AF_INET;
    listen_address.sin_addr.s_addr = listen_addr;
    listen_address.sin_port = listen_port;

    // Address/port descriptions for outgoing connection
    in_addr_t target_addr = htonl((((127 << 8) | 0) << 8 | 0) << 8 | 1);
    in_port_t target_port = htons(5905);

    sockaddr_in target_address;
    memset(&target_address, 0, sizeof(target_address));

    target_address.sin_family = AF_INET;
    target_address.sin_port = target_port;
    target_address.sin_addr.s_addr = target_addr;

    // Set up two tcp connections
    Socket destination(PF_INET, SOCK_STREAM, 0);
    Socket listen_socket(PF_INET, SOCK_STREAM, 0);

    try
    {

        // Set sockets to be non-blocking
        listen_socket.set_nonblocking(true);
        destination.set_nonblocking(true);
        // Don't wait for timeout on listen port; just reuse it
        listen_socket.set_reuse(true);

        // Bind on the listen port
        listen_socket.bind_to(listen_address);

        // Listen for incoming connections
        cout << "Listening for incoming data" << endl;
        listen_socket.listen(32);

        // Enter the main loop
        do
        {
            // Block for an incoming connection
            cout << "Awaiting incoming connection" << endl;
            wait_for_read(&listen_socket);
            Socket incoming = listen_socket.accept(NULL, NULL);
            cout << "Accepted incoming connection" << endl;

            // Establish connection to target 
            cout << "Connecting to server..." << endl;
            destination.connect(target_address); 


            // We select on vectors of sockets
            vector<const Socket*> sockets;
            sockets.push_back(&destination);
            sockets.push_back(&incoming);

            // Ready to pipe data
            bool round_over = false;
            do
            {
                cerr << "doing select for read data" << endl;
                auto readable = select_for_reading(sockets);

                for (auto r_socket : readable)
                {
                    // Get pointers to sockets back
                    if (r_socket == &incoming)
                    {
                        forward(incoming, destination, round_over);
                    }
                    else if (r_socket == &destination)
                    {
                        forward(destination, incoming, round_over);
                    }
                    else
                        cerr << "Unknown socket found in read set!" << endl;
                }

            } while (!round_over);

            // Set up a new Socket to connect to target - old one is dead.
            destination = Socket(PF_INET, SOCK_STREAM, 0);
        } while (true); // Until termination
    }
    catch (unique_ptr<SocketException>&& e)
    {
        const Socket* s = e->retrieve_socket();
        if (s != NULL)
        {
            cerr << "Problem in Socket " << s->identify() << ": ";
            if (s->is_listening())
                cerr << "[Listening] ";
            else
                cerr << "[Not Listening] ";
        }
        cerr << e->what() << endl;
    }
}
