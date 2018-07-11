#include <pservlet.h>
#include <pstd.h>
#include <stdio.h>
#include <string.h>
typedef struct {
	pipe_t               input, output;
	pstd_type_model_t*   type_model;
	pstd_type_accessor_t acc_a, acc_b;
} ctx_t;

static int init(uint32_t argc, char const* const* argv, void* ctxbuf) {
    ctx_t *ctx = (ctx_t*)ctxbuf;
    ctx->input  = pipe_define("input", PIPE_INPUT, "plumber_intro/typed_guest/adder_input");
    ctx->output = pipe_define("output", PIPE_OUTPUT, NULL);

	ctx->type_model = pstd_type_model_new();
	ctx->acc_a = pstd_type_model_get_accessor(ctx->type_model, ctx->input, "a");  // Get the accessor for input.a
	ctx->acc_b = pstd_type_model_get_accessor(ctx->type_model, ctx->input, "b");  // Get the accessor for input.b

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
	
	pstd_type_instance_t* type_inst = PSTD_TYPE_INSTANCE_LOCAL_NEW(ctx->type_model);

	uint32_t a = PSTD_TYPE_INST_READ_PRIMITIVE(uint32_t, type_inst, ctx->acc_a);  // Read input.a
	uint32_t b = PSTD_TYPE_INST_READ_PRIMITIVE(uint32_t, type_inst, ctx->acc_b);  // Read input.b

	snprintf(buf, sizeof(buf), "%u\n", a + b);
	pipe_write(ctx->output, buf, strlen(buf));

	pstd_type_instance_free(type_inst);

    return 0;
}

SERVLET_DEF = {
    .desc = "Add two numbers and output",
    .size = sizeof(ctx_t),
    .init = init,
    .exec = exec,
	.unload = unload
};
