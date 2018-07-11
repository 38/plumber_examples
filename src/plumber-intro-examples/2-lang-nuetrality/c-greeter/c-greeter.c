#include <pservlet.h>
static int init(uint32_t argc, char const* const* argv, void* ctxbuf) {
    pipe_t *ctx = (pipe_t*)ctxbuf;
    ctx[0] = pipe_define("input", PIPE_INPUT, NULL);
    ctx[1] = pipe_define("output", PIPE_OUTPUT, NULL);
    return 0;
}
static int exec(void* ctxbuf) {
    char name[1024] = {};
    pipe_t *ctx = (pipe_t*)ctxbuf;
    size_t sz = pipe_read(ctx[0], name, sizeof(name));
    pipe_write(ctx[1], name, sz);
    pipe_write(ctx[1], "hello from C\n", 13);
    return 0;
}
SERVLET_DEF = {
    .desc = "Say hello",
    .size = sizeof(pipe_t) * 2,
    .init = init,
    .exec = exec
};
