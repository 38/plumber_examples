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
	pstd_type_accessor_t x0;
	pstd_type_accessor_t y0;
	pstd_type_accessor_t z0;
	pstd_type_accessor_t x1;
	pstd_type_accessor_t y1;
	pstd_type_accessor_t z1;
	pstd_type_accessor_t x2;
	pstd_type_accessor_t y2;
	pstd_type_accessor_t z2;
} context_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;
	ctx->input = pipe_define("in", PIPE_INPUT, "graphics/FlattenColoredTriangle3D");
	ctx->output = pipe_define("out", PIPE_OUTPUT | PIPE_ASYNC, NULL);

	ctx->model = pstd_type_model_new();
	ctx->ar =pstd_type_model_get_accessor(ctx->model, ctx->input, "color.r");
	ctx->ag =pstd_type_model_get_accessor(ctx->model, ctx->input, "color.g");
	ctx->ab =pstd_type_model_get_accessor(ctx->model, ctx->input, "color.b");
	
	ctx->x0 =pstd_type_model_get_accessor(ctx->model, ctx->input, "x0");
	ctx->y0 =pstd_type_model_get_accessor(ctx->model, ctx->input, "y0");
	ctx->z0 =pstd_type_model_get_accessor(ctx->model, ctx->input, "z0");
	ctx->x1 =pstd_type_model_get_accessor(ctx->model, ctx->input, "x1");
	ctx->y1 =pstd_type_model_get_accessor(ctx->model, ctx->input, "y1");
	ctx->z1 =pstd_type_model_get_accessor(ctx->model, ctx->input, "z1");
	ctx->x2 =pstd_type_model_get_accessor(ctx->model, ctx->input, "x2");
	ctx->y2 =pstd_type_model_get_accessor(ctx->model, ctx->input, "y2");
	ctx->z2 =pstd_type_model_get_accessor(ctx->model, ctx->input, "z2");

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
	pstd_bio_printf(out, "R=%lf, G=%lf, B=%lf\n", r, g, b);
	
	float x = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->x0);
	float y = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->y0);
	float z = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->z0);
	pstd_bio_printf(out, "x=%lf, y=%lf, z=%lf\n", x, y, z);
	
	x = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->x1);
	y = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->y1);
	z = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->z1);
	pstd_bio_printf(out, "x=%lf, y=%lf, z=%lf\n", x, y, z);
	
	x = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->x2);
	y = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->y2);
	z = PSTD_TYPE_INST_READ_PRIMITIVE(float, inst, ctx->z2);
	pstd_bio_printf(out, "x=%lf, y=%lf, z=%lf\n", x, y, z);

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
