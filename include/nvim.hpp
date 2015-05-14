
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

template<typename response_handler, typename event_handler> class BaseAsyncNvimClient {
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
                boost::bind(&BaseAsyncNvimClient::handle_read, this, _1, _2));
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
        // TODO: typesafe
        const msgpack::object_array & res = result.get().via.array;
        auto msg = res.ptr;
        if (msg[0].as<int>() == RESPONSE) {
            uint64_t myid = msg[1].as<uint64_t>();
            auto it = waiting.find(myid);
            if (msg[2].type != msgpack::type::NIL) {
                cerr << msg[2] << endl;
            }
            it->second(msg[3]); //TODO: error
            waiting.erase(it);
        }
    }

public:
    template<typename... Args> void request_async(const char* name, response_handler &&handler, Args... args) {
        uint64_t myid = sid++;
        std::stringstream sb;
        msgpack::packer<decltype(sb)> p(sb);
        waiting.emplace(myid, handler);
        p.pack_array(4);
        p.pack_int(REQUEST).pack(myid).pack(name);
        // TODO: we might need to template this to make the handling of typed arrays safe.
        p.pack(std::tuple<Args...>(args...));
        asio::write(s, asio::buffer(sb.str()));
    }

    void eval(std::string code, response_handler &&handler) { request_async("vim_eval", std::move(handler), code); };

    bool connect(const std::string address) {
        s.connect(stream_protocol::endpoint(address));
    }

    void run() {
        running = true;
        read_more();
        io.run();
    }

    void stop() {
        running = false;
    }

};

class AsyncNvimClient : public BaseAsyncNvimClient<std::function<void(msgpack::object &)>, int> {
};

