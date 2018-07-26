#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pservlet.h>
#include <pstd.h>
#include <proto.h>
typedef struct {
	uint32_t in_cnt;
	uint32_t*  offset;
	pipe_t*  inputs;
	uint32_t   output_size;
	uint32_t   output_value_offset;
	uint32_t   output_id_offset;
	pipe_t   output;
} context_t;

static int _on_type_inferred(pipe_t pipe, const char* typename, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;
	uint32_t i;
	for(i = 0; i < ctx->in_cnt && ctx->inputs[i] != pipe; i ++);

	if(i < ctx->in_cnt)
	{

		proto_init();

		uint32_t sz;

		if(ERROR_CODE(size_t) == (ctx->offset[i] = proto_db_type_offset(typename, "value", &sz)))
			ERROR_RETURN_LOG(int, "libproto returns an error");

		if(sz != sizeof(uint32_t))
			ERROR_RETURN_LOG(int, "%s.value should have type uint32", typename);

		proto_finalize();
	}

	return 0;
}

static int _on_out_type_inferred(pipe_t pipe, const char* typename, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	proto_init();

	uint32_t sz;

	if(ERROR_CODE(size_t) == (ctx->output_value_offset = proto_db_type_offset(typename, "picked_value", &sz)))
		ERROR_RETURN_LOG(int, "Cannot get the offest of %s.picked_value", typename);

	if(sz != sizeof(uint32_t))
		ERROR_RETURN_LOG(int, "%s.picked_up should have type uint32", typename);

	if(ERROR_CODE(size_t) == (ctx->output_id_offset = proto_db_type_offset(typename, "picked_id", &sz)))
		ERROR_RETURN_LOG(int, "Cannot get the offest of %s.picked_id", typename);

	if(sz != sizeof(uint32_t))
		ERROR_RETURN_LOG(int, "%s.picked_up should have type uint32", typename);

	/* Although we can use the proto_db_type_size function to get the type of the size, however, we also have
	 * the mechanism which fill zeros autoamticall if the header is not written completely.
	 * So this is a example, we use the autoamtic zero filling for the last param */
	ctx->output_size = ctx->output_value_offset;
	if(ctx->output_size < ctx->output_id_offset) ctx->output_size = ctx->output_id_offset;
	ctx->output_size += sizeof(uint32_t);

	proto_finalize();
	return 0;
}

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	if(argc != 2)
		ERROR_RETURN_LOG(int, "Usage %s <#-of-inputs>", argv[0]);

	ctx->in_cnt = (uint32_t)atoi(argv[1]);

	ctx->inputs = (pipe_t*)malloc(ctx->in_cnt * sizeof(ctx->inputs[0]));
	ctx->offset = (uint32_t*)malloc(ctx->in_cnt * sizeof(ctx->offset[0]));

	memset(ctx->offset, -1, sizeof(ctx->offset[0]) * ctx->in_cnt);

	uint32_t i;
	for(i = 0; i < ctx->in_cnt; i ++)
	{
		char typebuf[32];
		snprintf(typebuf, sizeof(typebuf), "$T%u", i);
		ctx->inputs[i] = pipe_define_pattern("in%u", PIPE_INPUT, typebuf, i);
		pipe_set_type_callback(ctx->inputs[i], _on_type_inferred, ctxbuf);
	}

	ctx->output = pipe_define("out", PIPE_OUTPUT, "plumber_example/typed_pipe/Value");
	pipe_set_type_callback(ctx->output, _on_out_type_inferred, ctxbuf);

	return 0;
}

static int _exec(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	uint32_t i;
	for(i = 0; i < ctx->in_cnt; i ++)
		if(0 == pipe_eof(ctx->inputs[i])) break;

	if(i < ctx->in_cnt)
	{
		char data[ctx->offset[i] + sizeof(uint32_t)];

		pipe_hdr_read(ctx->inputs[i], data, ctx->offset[i] + sizeof(uint32_t));

		char outbuf[ctx->output_size];
		memset(outbuf, 0, ctx->output_size);
		*(uint32_t*)(outbuf + ctx->output_value_offset) = *(uint32_t*)(data + ctx->offset[i]);
		*(uint32_t*)(outbuf + ctx->output_id_offset)    = i;

		pipe_hdr_write(ctx->output, outbuf, ctx->output_size);
	}

	return 0;
}

static int _clean(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	if(ctx->offset != NULL) free(ctx->offset);
	if(ctx->inputs != NULL) free(ctx->inputs);

	return 0;

}

SERVLET_DEF = {
	.desc    = "Pickup the first pipe that has data in it and output the value of val in the header",
	.version = 0x0,
	.size    = sizeof(context_t),
	.init    = _init,
	.exec    = _exec,
	.unload  = _clean
};
