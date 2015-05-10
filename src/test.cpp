#include <iostream>
#include <iomanip>
#include <sstream>
#include <msgpack.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using std::cerr;
using std::endl;
namespace asio = boost::asio;
using asio::local::stream_protocol;

enum {
    REQUEST = 0,
    RESPONSE = 1,
    NOTIFY = 2
};

template<typename response_handler, typename event_handler> class AsyncNvimClient {
    asio::io_service io;
    // TODO: virtualize the socket, to allow multible connection types
    stream_protocol::socket s{io};
    msgpack::unpacker up;
    uint64_t sid = 0;
    std::unordered_map<uint64_t, response_handler> waiting;
    bool running = false;

    void read_more() {
        up.reserve_buffer(32*1024ul);
        s.async_read_some(asio::buffer(up.buffer(), up.buffer_capacity()),
                boost::bind(&AsyncNvimClient::handle_read, this, _1, _2));
    }

    void handle_read(const boost::system::error_code& error, size_t read) {
        if (error) {
            exit(1);
        }
        msgpack::unpacked result;
        up.buffer_consumed(read);
        while (up.next(result)) {
            handle_message(result);
        }
        if (running) read_more();
    }

    void handle_message(msgpack::unpacked &result) {
        cerr << result.get() << endl;
        running = false;
    }

public:
    template<typename... Args> void request_async(std::string name, std::tuple<Args...> args) {
        std::stringstream sb;
        std::tuple<int, int, std::string, std::tuple<Args...>> t(REQUEST, sid++, name, args);
        msgpack::pack(sb,t);
        asio::write(s, asio::buffer(sb.str()));
    }


    bool connect(const std::string address) {
        s.connect(stream_protocol::endpoint(address));
    }

    void run() {
        running = true;
        read_more();
        io.run();
    }

};

// placeholder
class NvimClient : public AsyncNvimClient<int,int> {
};

int main() {
    NvimClient nv;
    char* addr = getenv("NVIM_LISTEN_ADDRESS");
    if(!addr) return 0;
    nv.connect(addr);
    nv.request_async("vim_get_api_info", {});
    nv.run();
}

/*
noremap <Plug>ch:un :up<cr><c-w><c-w>imake run<cr><c-\><c-n><c-w><c-w>
*/
