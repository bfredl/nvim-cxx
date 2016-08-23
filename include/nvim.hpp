#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <msgpack.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using std::cerr;
using std::endl;
using std::move;
namespace asio = boost::asio;
using asio::local::stream_protocol;

enum {
    REQUEST = 0,
    RESPONSE = 1,
    NOTIFY = 2
};

template<typename response_handler, typename event_handler> class BaseNvimClient {
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
                boost::bind(&BaseNvimClient::handle_read, this, _1, _2));
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
                //cerr << msg[2] << endl;
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
        waiting.emplace(myid, move(handler));
        p.pack_array(4);
        p.pack_int(REQUEST).pack(myid).pack(name);
        // TODO: we might need to template this to make the handling of typed arrays safe.
        p.pack(std::tuple<Args...>(args...));
        //cerr << sb.str();
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

    void stop() {
        running = false;
    }

};

// unityped objects for now
struct NvimObject {
    int8_t tag;
    //uint64_t id;
    int8_t len;
    char data[9];
    NvimObject( msgpack::object & o) {
        assert(o.type == msgpack::type::EXT);
        auto & ext = o.via.ext;
        tag = ext.type();
        // TODO: unpack the nested msgpack number
        len = (int8_t)ext.size;
        assert(len <= 9);
        memcpy(data, ext.data(), len);
    }

    template <typename Packer>
    void msgpack_pack(Packer& pk) const {
        pk.pack_ext(len, tag);
        pk.pack_ext_body(data, len);
    }
};

std::ostream& operator<<(std::ostream& os, const NvimObject& o)
{
    os << "NvimObject(" << (int)o.tag << ")";
    return os;
}

using objhandler = std::function<void(msgpack::object&)>;

template<class T> struct _handler_t {
    using handler_t = std::function<void(T)>;

    static objhandler convert_return(handler_t && handler) {
        return [handler](msgpack::object &o) { handler(o.as<T>()); };
    }
};

template<> struct _handler_t<void> {
    // std::function cannot <void(void)>
    using handler_t = std::function<void()>;
    static objhandler convert_return(handler_t && handler) {
        return [handler](msgpack::object &o) { (void)o; handler(); };
    }
};

// TODO: probably should replace this with a NvimObject ADT
template<> struct _handler_t<msgpack::object&> {
    using handler_t = objhandler;
    static objhandler convert_return(handler_t && handler) {
        return [handler](msgpack::object &o) { handler(o); };
    }
};

template<> struct _handler_t<NvimObject> {
    using handler_t = std::function<void(NvimObject)>;
    static objhandler convert_return(handler_t && handler) {
        return [handler](msgpack::object &o) { handler(NvimObject(o)); };
    }
};

class HandlerNvimClient : public BaseNvimClient<std::function<void(msgpack::object &)>, int> {
protected:
    template<class T> using handler_t = typename _handler_t<T>::handler_t;

    template<class T, typename... Args> void request(const char* name, handler_t<T> &&handler, Args... args) {
        request_async(name, _handler_t<T>::convert_return(move(handler)), args...);
    }

};

/*
class NvimApiClient : public HandlerNvimClient {
public:
void eval(std::string a_str, handler_t<msgpack::object&> && handler)
    { request<msgpack::object&>("vim_eval", move(handler), a_str); }

void strwidth(std::string a_str, handler_t<int64_t> && handler)
    { request<int64_t>("vim_strwidth", move(handler), a_str); }

};
*/

#include "api_gen.hpp"
