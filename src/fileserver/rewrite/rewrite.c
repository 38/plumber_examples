/**
 * Copyright (C) 2018 Hao Hou
 **/
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <pstd.h>
#include <pstd/types/string.h>
#include <pservlet.h>

typedef struct {
	pipe_t                  p_input;      /*!< The path to request */
	pipe_t                  p_output;     /*!< The request data structure used by proxy servlet */

	pstd_type_model_t*      type_model;   /*!< The type model */
	pstd_type_accessor_t    a_host;       /*!< The host accessor */
	pstd_type_accessor_t    a_base;       /*!< The relative URL accessor */

	char*                   host;         /*!< The new host name */
	char*                   base;         /*!< The new base dir */
} ctx_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxmem)
{
	if(argc < 3) 
		ERROR_RETURN_LOG(int, "Invalid servlet init param");

	ctx_t* ctx = (ctx_t*)ctxmem;

	memset(ctx, 0, sizeof(ctx_t));

	ctx->p_input = pipe_define("input", PIPE_INPUT, "plumber/std_servlet/network/http/parser/v0/RequestData");
	ctx->p_output = pipe_define("output", PIPE_OUTPUT, "plumber/std_servlet/network/http/parser/v0/RequestData");

	ctx->host = strdup(argv[1]);
	ctx->base = strdup(argv[2]);

	PSTD_TYPE_MODEL(type_list)
	{
		PSTD_TYPE_MODEL_FIELD(ctx->p_output, host.token, ctx->a_host),
		PSTD_TYPE_MODEL_FIELD(ctx->p_output, base_url.token, ctx->a_base) 
	};

	ctx->type_model = PSTD_TYPE_MODEL_BATCH_INIT(type_list);

	pstd_type_model_copy_pipe_data(ctx->type_model, ctx->p_input, ctx->p_output);

	return 0;
}

static int _unload(void* ctxmem)
{
	ctx_t* ctx = (ctx_t*)ctxmem;
	if(NULL != ctx->type_model) pstd_type_model_free(ctx->type_model);
	if(NULL != ctx->base) free(ctx->base);
	if(NULL != ctx->host) free(ctx->host);

	return 0;
}

static int _exec(void* ctxmem)
{
	ctx_t* ctx = (ctx_t*)ctxmem;

	pstd_type_instance_t* inst = PSTD_TYPE_INSTANCE_LOCAL_NEW(ctx->type_model);

	pstd_string_create_commit_write(inst, ctx->a_host, ctx->host);
	pstd_string_create_commit_write(inst, ctx->a_base, ctx->base);

	pstd_type_instance_free(inst);

	return 0;
}

SERVLET_DEF = {
	.desc = "Construct a reverse proxy request based on the input path",
	.version = 0x0,
	.size = sizeof(ctx_t),
	.init = _init,
	.exec = _exec,
	.unload = _unload
};
