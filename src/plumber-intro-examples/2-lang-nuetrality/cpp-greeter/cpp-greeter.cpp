#include <new>
#include <pservlet.h>
struct Context {
    pipe_t name, output;
    Context() : name(pipe_define("input", PIPE_INPUT, NULL)), 
                output(pipe_define("output", PIPE_OUTPUT, NULL)) {}
    void Hello() {
        char buf[1024] = {};
        size_t sz = pipe_read(name, buf, 1024);
        pipe_write(output, buf, sz);
        pipe_write(output, "hello from C++\n", 15);
    }
};
static int init(uint32_t argc, char const* const* argv, void* ctxbuf) {
    new (ctxbuf) Context();
    return 0;
}
static int exec(void* ctxbuf) {
    ((Context*)ctxbuf)->Hello();
    return 0;
}
PSERVLET_EXPORT(Context, init, exec, NULL, "Say hello");
