#include <pservlet.h>
#include <pstd.h>
#include <proto.h>

typedef struct {
	uint32_t size;
	uint32_t id;
	uint32_t value;
	uint32_t zero;
	pipe_t input;
	pipe_t output;
} context_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;
	ctx->input = pipe_define("in", PIPE_INPUT, "plumber_example/typed_pipe/Value");
	ctx->output = pipe_define("out", PIPE_OUTPUT | PIPE_ASYNC, NULL);

	proto_init();

	ctx->size = proto_db_type_size("plumber_example/typed_pipe/Value");
	ctx->id   = proto_db_type_offset("plumber_example/typed_pipe/Value", "picked_id", NULL);
	ctx->value = proto_db_type_offset("plumber_example/typed_pipe/Value", "picked_value", NULL);
	ctx->zero = proto_db_type_offset("plumber_example/typed_pipe/Value", "zero", NULL);

	proto_finalize();

	return 0;
}

static int _exec(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;
	char buf[ctx->size];
	pipe_hdr_read(ctx->input, buf, ctx->size);

	pstd_bio_t* out = pstd_bio_new(ctx->output);

	pstd_bio_printf(out, "You have pciked %u, value is %u, zero is %u\r\n", *(uint32_t*)(buf + ctx->id), *(uint32_t*)(buf + ctx->value), *(uint32_t*)(buf + ctx->zero));

	pstd_bio_free(out);

	return 0;
}

static int _unload(void* ctxbuf)
{
	(void)ctxbuf;
	return 0;
}

SERVLET_DEF = {
	.desc = "The servlet writes socket",
	.version = 0x0,
	.size = sizeof(context_t),
	.init = _init,
	.exec = _exec,
	.unload =  _unload
};
