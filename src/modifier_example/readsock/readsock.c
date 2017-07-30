#include <stdlib.h>
#include <time.h>

#include <pservlet.h>
#include <pstd.h>
#include <proto.h>

typedef struct {
	pipe_t in;
	pipe_t out;
	pipe_t bout;
	pstd_type_model_t* model;
	pstd_type_accessor_t ar;
	pstd_type_accessor_t ag;
	pstd_type_accessor_t ab;

	pstd_type_accessor_t abo;
} context_t;

#define _CHK(what) if((what) == ERROR_CODE(typeof(what))) return ERROR_CODE(int);

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	ctx->in = pipe_define("in", PIPE_INPUT, NULL);
	ctx->out = pipe_define("out", PIPE_OUTPUT, "graphics/ColoredTriangle3D");
	ctx->bout = pipe_define("bout", PIPE_OUTPUT, "float");

	ctx->model = pstd_type_model_new();
	ctx->ar = pstd_type_model_get_accessor(ctx->model, ctx->out, "color.r");
	ctx->ag = pstd_type_model_get_accessor(ctx->model, ctx->out, "color.g");
	ctx->ab = pstd_type_model_get_accessor(ctx->model, ctx->out, "color.b");

	ctx->abo = pstd_type_model_get_accessor(ctx->model, ctx->bout, "value");
	return 0;
}

static int _exec(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	size_t ti = pstd_type_instance_size(ctx->model);
	char buf[ti];
	pstd_type_instance_t* inst = pstd_type_instance_new(ctx->model, buf);

	char ignored[4096];
	pipe_read(ctx->in, ignored, sizeof(ignored));
	
	PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->ar, (float)0.1);
	PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->ag, (float)0.2);
	PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->ab, (float)0.3);

	PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->abo, (float)1e10);

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
