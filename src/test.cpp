#include <iostream>
#include <iomanip>
#include <sstream>
#include <msgpack.hpp>
#include <boost/asio.hpp>

using std::cerr;
using std::endl;
namespace asio = boost::asio;
using asio::local::stream_protocol;

enum {
    REQUEST = 0,
    RESPONSE = 1,
    NOTIFY = 2
};

class NvimClient {
    asio::io_service io;
    // TODO: virtualize the socket, to allow multible connection types
    stream_protocol::socket s{io};
    msgpack::unpacker up;
    uint64_t sid = 0;

    // synchronous test
    bool next_message(msgpack::unpacked& result) {
        if (up.next(result)) {
            return true;
        }
        size_t avail;
        while(avail = s.available()) {
            up.reserve_buffer(std::min(32*1024ul,avail));
            boost::system::error_code error;
            size_t read = s.read_some(asio::buffer(up.buffer(), up.buffer_capacity()), error);
            up.buffer_consumed(read);
            if (up.next(result)) {
                return true;
            }
        }
        return false;
    }


public:
    bool connect(const std::string address) {
        s.connect(stream_protocol::endpoint(address));
        std::stringstream sb;
        std::tuple<int, int, std::string, std::tuple<>> t(REQUEST, sid++, "vim_get_api_info", {});
        msgpack::pack(sb,t);
        asio::write(s, asio::buffer(sb.str()));
        msgpack::unpacked result;
        while(!next_message(result)) {} //sync test
        cerr << result.get() << endl;
    }
};

int main() {
    NvimClient nv;
    char* addr = getenv("NVIM_LISTEN_ADDRESS");
    if(!addr) return 0;
    nv.connect(addr);
}

/*
noremap <Plug>ch:un :up<cr><c-w><c-w>:terminal make run<cr><c-\><c-n>:set bufhidden=wipe<cr><c-w><c-w>
noremap <Plug>ch:un :up<cr><c-w><c-w>imake run<cr><c-\><c-n><c-w><c-w>
*/
