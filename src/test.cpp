#include <iostream>
#include <msgpack.hpp>
#include <boost/asio.hpp>

using std::cout;
using std::endl;
namespace asio = boost::asio;

class NvimClient {
    asio::io_service io;
};

int main() {
    NvimClient nv;
    cout << "test!" << endl;
}
