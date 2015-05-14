#include "../include/nvim.hpp"

int main() {
    AsyncNvimClient nv;
    char* addr = getenv("NVIM_LISTEN_ADDRESS");
    if(!addr) return 0;
    nv.connect(addr);
    //nv.request_async("vim_get_api_info", [](msgpack::object& res) {cerr << res << endl;} );
    nv.strwidth("bred sträng", [](int64_t res) {cerr << res << endl;});
    nv.request_async("vim_get_current_buffer", [&](msgpack::object& buf) {
        nv.request_async("buffer_get_line", [&](msgpack::object& res) {cerr << res << endl; nv.stop();}, buf, 0 );
    });
    /*nv.eval("2+2", [&](msgpack::object& res) {
        cerr << res << endl;
        nv.request_async("vim_get_current_line", [&](msgpack::object& res) {cerr << res << endl; nv.stop();} );
    });*/
    nv.run();
}

/*
noremap <Plug>ch:un :up<cr><c-w><c-w>imake run<cr><c-\><c-n><c-w><c-w>
*/
