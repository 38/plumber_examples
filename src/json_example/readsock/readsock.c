#include <stdlib.h>
#include <time.h>

#include <pservlet.h>
#include <pstd.h>
#include <pstd/types/string.h>
#include <proto.h>

typedef struct {
	pipe_t in;
	pipe_t out;
	pipe_t bout;
	pstd_type_model_t* model;
	pstd_type_accessor_t token_acc;
} context_t;

#define _CHK(what) if((what) == ERROR_CODE(typeof(what))) return ERROR_CODE(int);

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	ctx->in = pipe_define("in", PIPE_INPUT, NULL);
	ctx->out = pipe_define("out", PIPE_OUTPUT, "plumber/std/request_local/String");

	ctx->model = pstd_type_model_new();
	ctx->token_acc = pstd_type_model_get_accessor(ctx->model, ctx->out, "token");
	return 0;
}

static int _exec(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	size_t ti = pstd_type_instance_size(ctx->model);
	char buf[ti];
	pstd_type_instance_t* inst = pstd_type_instance_new(ctx->model, buf);

	char data[4096];
	size_t sz = pipe_read(ctx->in, data, sizeof(data));

	pstd_string_t* str = pstd_string_new(4096);
	pstd_string_write(str, data, sz);
	scope_token_t tk = pstd_string_commit(str);
	PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->token_acc, tk);
	pstd_type_instance_free(inst);
	return 0;
}

static inline int _cleanup(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	pstd_type_model_free(ctx->model);
	return 0;
}

SERVLET_DEF = {
	.desc = "Read data from socket",
	.version = 0x0,
	.size = sizeof(context_t),
	.init = _init,
	.exec = _exec,
	.unload = _cleanup
};
