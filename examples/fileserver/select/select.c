/**
 * Copyright (C) 2017, Hao Hou
 **/
#include <pservlet.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
	uint32_t     nconds;
	pipe_t       selector;
	pipe_t       input;
	pipe_array_t* outputs;
	pipe_t       def;
	char const** conds;
} context_t;

static int _init(uint32_t argc, char const* const* argv, void* buf)
{
	context_t* ctx = (context_t*)buf;
	ctx->nconds = argc - 1;

	ctx->selector = pipe_define("selector", PIPE_INPUT, NULL);
	ctx->input    = pipe_define("input", PIPE_INPUT, NULL);
	ctx->outputs  = pipe_array_new("output_#", PIPE_OUTPUT | PIPE_SHADOW | PIPE_DISABLED | PIPE_GET_ID(ctx->input), NULL, 0, (int)ctx->nconds);
	ctx->def      = pipe_define("default", PIPE_OUTPUT | PIPE_SHADOW | PIPE_DISABLED | PIPE_GET_ID(ctx->input), NULL);

	ctx->conds = (char const* *)calloc(1, sizeof(char const*) * ctx->nconds);

	uint32_t i;
	for(i = 0; i < ctx->nconds; i ++)
	    ctx->conds[i] = argv[i + 1];

	return 0;
}

static int _exec(void* buf)
{
	context_t* ctx = (context_t*)buf;
	uint32_t i;
	char input[1024];
	size_t sz = pipe_read(ctx->selector, input, sizeof(input));
	input[sz] = 0;
	pipe_t out_pipe = ctx->def;

	for(i = 0; i < ctx->nconds; i ++)
	    if(strcmp(ctx->conds[i], input) == 0)
	    {
		    out_pipe = pipe_array_get(ctx->outputs, i);
		    break;
	    }

	pipe_cntl(out_pipe, PIPE_CNTL_CLR_FLAG, PIPE_DISABLED);

	return 0;
}

static int _unload(void* buf)
{
	context_t* ctx = (context_t*)buf;
	free(ctx->conds);
	pipe_array_free(ctx->outputs);

	return 0;
}

SERVLET_DEF = {
	.desc = "The Demux selector",
	.version = 0,
	.size = sizeof(context_t),
	.init = _init,
	.exec = _exec,
	.unload = _unload
};
