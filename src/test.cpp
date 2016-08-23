#include "../include/nvim.hpp"

int main() {
    NvimApiClient nv;
    char* addr = getenv("NVIM_LISTEN_ADDRESS");
    if(!addr) return 0;
    nv.connect(addr);
    std::stringstream sb;
    //nv.request_async("vim_get_api_info", [](msgpack::object& res) {cerr << res << endl;} );
    nv.strwidth("bred sträng", [](int64_t res) {cerr << res << endl;});
    nv.get_current_buffer([&](NvimObject buf) {
        cerr << buf << endl;
        nv.buffer_get_line(buf, 0, [&](std::string res) {cerr << res << endl; nv.stop();} );
    });
    /*nv.eval("2+2", [&](msgpack::object& res) {
        cerr << res << endl;
        nv.request_async("vim_get_current_line", [&](msgpack::object& res) {cerr << res << endl; nv.stop();} );
    });*/
    nv.run();
}

/*
new | let g:bterm = termopen(&shell)
noremap <Plug>ch:un :up<cr>:call jobsend(g:bterm, "make run\n")<cr>
*/
