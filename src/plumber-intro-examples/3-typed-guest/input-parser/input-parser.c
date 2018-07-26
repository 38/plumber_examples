#include <pservlet.h>
#include <pstd.h>
#include <stdio.h>
typedef struct {
	pipe_t               input, output;
	pstd_type_model_t*   type_model;
	pstd_type_accessor_t acc_a, acc_b;
} ctx_t;

static int init(uint32_t argc, char const* const* argv, void* ctxbuf) {
	ctx_t *ctx = (ctx_t*)ctxbuf;
	ctx->input  = pipe_define("input", PIPE_INPUT, NULL);
	ctx->output = pipe_define("output", PIPE_OUTPUT, "plumber_intro/typed_guest/adder_input");

	ctx->type_model = pstd_type_model_new();
	ctx->acc_a = pstd_type_model_get_accessor(ctx->type_model, ctx->output, "a");  // Get the accessor for output.a
	ctx->acc_b = pstd_type_model_get_accessor(ctx->type_model, ctx->output, "b");  // Get the accessor for output.b

	return 0;
}

static int unload(void* ctxbuf)
{
	ctx_t *ctx = (ctx_t*)ctxbuf;

	pstd_type_model_free(ctx->type_model);

	return 0;
}

static int exec(void* ctxbuf) {
	char buf[1024] = {};
	ctx_t *ctx = (ctx_t*)ctxbuf;
	size_t sz = pipe_read(ctx->input, buf, sizeof(buf));
	buf[sz] = 0;
	int a, b;
	sscanf(buf, "%u%u", &a, &b);

	pstd_type_instance_t* type_inst = PSTD_TYPE_INSTANCE_LOCAL_NEW(ctx->type_model);

	PSTD_TYPE_INST_WRITE_PRIMITIVE(type_inst, ctx->acc_a, a);   // write a to output.a
	PSTD_TYPE_INST_WRITE_PRIMITIVE(type_inst, ctx->acc_b, b);   // write b to output.b

	pstd_type_instance_free(type_inst);

	return 0;
}

SERVLET_DEF = {
	.desc = "Parse the two numbers",
	.size = sizeof(ctx_t),
	.init = init,
	.exec = exec,
	.unload = unload
};
