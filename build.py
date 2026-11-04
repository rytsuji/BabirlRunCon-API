#!/usr/bin/env python3
"""Build only source objects; never use objects/binaries from the archive."""
import os, pathlib, shlex, subprocess, sys
def build_native(output=None):
    root = pathlib.Path(__file__).resolve().parent
    b = root / 'build' / 'native'
    b.mkdir(parents=True, exist_ok=True)
    cli = (root/'csrc/babirl/babicon/babicmdjson.c').read_text()
    cli = cli[:cli.index('int main(int argc,char *argv[])')]
    # Start errors may exceed the CLI's 272-byte local buffer.
    cli=cli.replace('      sprintf(js, "Can\'t start, because %s", buff+sizeof(ret));\n      bbjson_charobj("error", js);',
                    '      emit_start_error(buff+sizeof(ret));')
    cli=cli.replace('  char js[272] = {0};\n','',2)
    # Preserve the EB's Unix timestamps; local formatted strings lose timezone
    # information and render uninitialized zero as a real 1970 date.
    cli=cli.replace('bbjson_charobj("startdate", chst);','bbjson_charobj("startdate", chst); bbjson_intobj("starttime", runinfo.starttime);')
    cli=cli.replace('bbjson_charobj("stopdate", chst);','bbjson_charobj("stopdate", chst); bbjson_intobj("stoptime", runinfo.stoptime);')
    # stop() returns runinfo only; avoid the CLI's additional, unchecked ES RPCs.
    a = cli.index('int stop(char *arg)')
    z = cli.index('int setrunnumber(', a)
    cli = cli[:a] + cli[a:z].replace('  getevtn(NULL);', '  /* ES event counts are omitted in library mode. */') + cli[z:]
    cli = cli.replace('exit(0);', 'babi_abort("exit is unavailable in library mode");')
    a = cli.index('int getevtn(char *arg)')
    z = cli.index('int getesevtnumber(', a)
    cli = cli[:a] + 'int getevtn(char *arg){ return babi_get_es_events(); }\n\n' + cli[z:]
    (b/'client.c').write_text(cli)
    com = (root/'csrc/babirl/babicon/comfuncsjson.c').read_text()
    # The CLI closes the JSON document on failed connection and continues using fd 0.
    # Library mode unwinds immediately, leaving one complete error document.
    for name in ('babild', 'babinfo'):
        start = com.index('    bbjson_charobj("error", "babicon: Can\'t connet to '+name)
        end = com.index('    return 0;', start) + len('    return 0;')
        com = com[:start] + '    babi_abort("Cannot connect to '+name+'");' + com[end:]
    com = com.replace('mktcpsend(host, EBCOMPORT)', 'mktcpsend(host, babi_eb_port())')
    (b/'comfuncsjson.c').write_text(com)
    tcp = (root/'csrc/babirl/lib/bi-tcp.c').read_text()
    if sys.platform == 'darwin':
        a = tcp.index('int getmacbysock(')
        z = tcp.index('int getipbysock(', a)
        tcp = tcp[:a] + 'int getmacbysock(int sock, char *mac){ return 0; }\n' + tcp[z:]
    tcp = tcp.replace('int mktcpsend(', 'int original_mktcpsend(').replace('int eb_get(', 'int original_eb_get(').replace('int eb_set(', 'int original_eb_set(')
    (b/'tcp.c').write_text(tcp)
    cc = shlex.split(os.environ.get('CC', 'cc'))
    flags = ['-std=gnu89', '-O2', '-fPIC', '-fvisibility=hidden', '-fcommon', '-DHUGEBUFF',
             '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE_SOURCE',
             '-Wno-implicit-function-declaration', '-Wno-int-conversion',
             '-Wno-incompatible-pointer-types', '-Wno-return-type',
             '-I'+str(root/'csrc/babirl/include'), '-I'+str(root/'csrc/babirl/babicon'),
             '-I'+str(b), '-include', str(root/'csrc/capture.h')]
    flags += shlex.split(os.environ.get('CPPFLAGS', ''))
    flags += shlex.split(os.environ.get('CFLAGS', ''))
    objects = []
    for src in [root/'csrc/binding.c', b/'tcp.c'] + [root/'csrc/babirl/lib'/s for s in
                                   ('bi-com.c','bbxml.c','bbjson.c')]:
        obj = b/(src.stem+'.o')
        subprocess.run(cc+flags+['-c',str(src),'-o',str(obj)],check=True)
        objects.append(str(obj))
    destination = pathlib.Path(output) if output is not None else root/'runcon'
    destination.mkdir(parents=True, exist_ok=True)
    lib = destination/('libbabicmd.dylib' if sys.platform == 'darwin' else 'libbabicmd.so')
    subprocess.run(cc+(['-dynamiclib'] if sys.platform == 'darwin' else ['-shared','-Wl,-Bsymbolic'])+
                   objects+shlex.split(os.environ.get('LDFLAGS',''))+
                   ['-lreadline','-lncurses','-lexpat','-lpthread','-o',str(lib)],check=True)
    return lib

if __name__ == '__main__':
    print(build_native())
