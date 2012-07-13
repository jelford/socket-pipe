#include <iostream>

#include <fstream>      // std::ofstream (file io)
#include <iomanip>      // setw
#include <memory>       // std::unique_ptr
#include <algorithm>   // std::find

#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

#include <execinfo.h>

#include "Socket.hpp"

using namespace std;
using namespace jelford;

template <typename LOGGER>
void forward(Socket& incoming, Socket& destination, bool& round_over, LOGGER& log)
{
    wait_for_read(&incoming);
    auto data = incoming.read();

    wait_for_write(&destination);
    destination.write(data);

    if (data.size() == 0)
        // Other end has hung up
        round_over = true;

    log.log(incoming.identify(), destination.identify(), data);
}

class LogException : public std::exception
{
    private:
        string msg;
    public:
        LogException(string msg) : msg(msg)
        {}

        virtual const char* what()
        {
            return msg.c_str();
        }
};

class Logger
{
    private:
        ofstream log_file;
        ostream* out;
    public:

        Logger() : out(&std::cerr)
        { }
        
        void open(std::string filename)
        {
            log_file.open(filename, ios::out);
            if (!log_file.is_open())
                throw LogException("Couldn't open " + filename + " for logging. Redirecting to STDERR.\n");
            else
                out = &log_file;
        }

        void log(int from, int to, std::vector<unsigned char> data)
        {
           
            *out << dec << from << " -> " << to << " [" << data.size() << "]:\t";
            for (auto d : data)
                *out << hex << setfill('0') << setw(2) << static_cast<int>(d) << ":";
            *out << "\n";

            out->flush();
        }

        template <typename T>
        Logger& operator<<(const T& data)
        {
            *out << data;
            out->flush();
            return *this;
        }

        virtual ~Logger()
        {
            log_file.flush();
            log_file.close();
        }

};


int main(int const argc, char const * const argv[])
{
    if (argc < 6)
    {
        cout << "Usage: " << endl << argv[0] << " LISTEN_ADDRESS LISTEN_PORT TARGET_ADDRESS TARGET_PORT " << endl;
        return EXIT_FAILURE;
    }

    Logger logger;
    try
    {
        logger.open(argv[5]);
    }
    catch (LogException e)
    {
        cerr << e.what();
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

        logger << "Incoming requests on: " << listen_socket.identify() << "\n";

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
            listen_socket.listen(32);

            // The main loop just awaits incoming connections, forwards
            // connects to the destination, forwards any traffic, then
            // discards the socket and awaits new incoming connections.
            do
            {

                // Block for an incoming connection
                wait_for_read(&listen_socket);
                Socket incoming = listen_socket.accept(NULL, NULL);

                // Establish connection to target 
                destination.connect(target_address); 
                logger << "New session (incoming/outgoing data on: " << incoming.identify() << "/" << destination.identify() << ")\n";


                // We select on vectors of sockets
                vector<const Socket*> sockets;
                sockets.push_back(&destination);
                sockets.push_back(&incoming);

                // Ready to pipe data
                bool round_over = false;
                do
                {
                    auto readable = select_for_reading(sockets);

                    for (auto r_socket : readable)
                    {
                        // Get pointers to sockets back
                        if (r_socket == &incoming)
                        {
                            forward(incoming, destination, round_over, logger);
                        }
                        else if (r_socket == &destination)
                        {
                            forward(destination, incoming, round_over, logger);
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
