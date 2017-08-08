#include <pservlet.h>
#include <pstd.h>
#include <proto.h>

#include <pstd/types/string.h>

typedef struct {
	pipe_t input;
	pipe_t output;
	pstd_type_model_t* model;
	pstd_type_accessor_t token_acc;
} context_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;
	ctx->input = pipe_define("in", PIPE_INPUT, "plumber/std/request_local/String");
	ctx->output = pipe_define("out", PIPE_OUTPUT | PIPE_ASYNC, NULL);

	ctx->model = pstd_type_model_new();
	ctx->token_acc = pstd_type_model_get_accessor(ctx->model, ctx->input, "token");
	return 0;
}

static int _exec(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	pstd_bio_t* out = pstd_bio_new(ctx->output);

	size_t ti = pstd_type_instance_size(ctx->model);
	char buf[ti];
	pstd_type_instance_t* inst = pstd_type_instance_new(ctx->model, buf);

	scope_token_t token = PSTD_TYPE_INST_READ_PRIMITIVE(scope_token_t, inst, ctx->token_acc);
	const pstd_string_t* ps = pstd_string_from_rls(token);
	const char* str = pstd_string_value(ps);

	pstd_bio_puts(out, str);

	pstd_type_instance_free(inst);

	pstd_bio_free(out);

	return 0;
}

static int _unload(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	pstd_type_model_free(ctx->model);
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
