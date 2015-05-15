from __future__ import print_function, division
import os, sys
import neovim

os.environ
addr = os.environ.get('NVIM_LISTEN_ADDRESS')
if addr:
    nvim = neovim.attach('socket', path=addr)
else:
    nvim = neovim.attach('child', argv=['nvim', '-u', 'NONE', '--embed'])

metadata = nvim.metadata
types = metadata['types']
errtypes = metadata['error_types']
funs = metadata['functions']

typemap = {
    'Integer': 'int64_t',
    'Float': 'double',
    'Boolean': 'bool',
    'String': 'std::string',
    'void': 'void'
}

def dump_function(f):
    arg_list = []
    template_list = []
    c_args = ['"{}"'.format(f['name']), 'move(handler)']
    for (typ, name) in f['parameters']:
        # escape, to avoid conflicts with keywords and variables
        cname = 'a_'+name
        ctype = typemap.get(typ)
        if not ctype:
            ctype = 'T_'+name
            template_list.append('typename '+ctype)
        arg_list.append(' '.join((ctype, cname)))
        c_args.append(cname)

    rettype = typemap.get(f['return_type'], 'msgpack::object&')
    arg_list.append('handler_t<{}> && handler'.format(rettype))
    recv, method = f['name'].split('_',1)
    methname = method if recv == 'vim' else f['name']
    decl = "void {}({})\n    {{ request<{}>({}); }}\n".format(methname, ', '.join(arg_list), rettype, ', '.join(c_args))
    if template_list:
        decl = "template<{}> {}".format(', '.join(template_list), decl)
    return decl

if len(sys.argv) >= 2:
    f = open(sys.argv[1], 'w')
else:
    f = sys.stdout

f.write("class NvimApiClient : public HandlerNvimClient {\n");
f.write("public:\n");

f.write(dump_function(funs[5])+"\n")
f.write(dump_function(funs[6])+"\n")

f.write("};\n")
f.close()

