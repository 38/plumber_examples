#include <pservlet.h>
#include <pstd.h>
#include <proto.h>

typedef struct {
	pipe_t input;
	pipe_t output;
	pstd_type_model_t* model;
	pstd_type_accessor_t ar;
	pstd_type_accessor_t ag;
	pstd_type_accessor_t ab;
} context_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;
	ctx->input = pipe_define("in", PIPE_INPUT, "graphics/ColorRGB");
	ctx->output = pipe_define("out", PIPE_OUTPUT | PIPE_ASYNC, NULL);

	ctx->model = pstd_type_model_new();
	ctx->ar =pstd_type_model_get_accessor(ctx->model, ctx->input, "r");
	ctx->ag =pstd_type_model_get_accessor(ctx->model, ctx->input, "g");
	ctx->ab =pstd_type_model_get_accessor(ctx->model, ctx->input, "b");

	return 0;
}

static int _exec(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	pstd_bio_t* out = pstd_bio_new(ctx->output);

	size_t ti = pstd_type_instance_size(ctx->model);
	char buf[ti];
	pstd_type_instance_t* inst = pstd_type_instance_new(ctx->model, buf);

	float r = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->ar);
	float g = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->ag);
	float b = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->ab);

	pstd_bio_printf(out, "R=%lf, G=%lf, B=%lf", r, g, b);

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
