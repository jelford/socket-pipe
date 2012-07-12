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

int main(int const argc, char const * const argv[])
{
    if (argc < 5)
    {
        cout << "Usage: " << endl << argv[0] << " LISTEN_ADDRESS LISTEN_PORT TARGET_ADDRESS TARGET_PORT " << endl;
        return EXIT_FAILURE;
    }

    // Hints for building both listen & connection addresses
    addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = PF_INET;      // UNSPEC results in INET6, but our router doesn't assign INET6 addresses (EADDRNOTAVAIL)

    try
    {
        Address listen_address = Address(argv[1], argv[2], hints);
        Address target_address = Address(argv[3], argv[4], hints);
    

        // Set up two tcp connections
        Socket destination(target_address.family, SOCK_STREAM, 0);
        Socket listen_socket(listen_address.family, SOCK_STREAM, 0);

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

            // The main loop just awaits incoming connections, forwards
            // connects to the destination, forwards any traffic, then
            // discards the socket and awaits new incoming connections.
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
    catch(AddressException e)
    {
        cerr << "Address exception: " << e.msg() << endl;
        return EXIT_FAILURE;
    }

}
