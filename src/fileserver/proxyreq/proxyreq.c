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
	pipe_t               path;        /*!< The path to request */
	pipe_t               request;     /*!< The request data structure used by proxy servlet */
	char*                base;        /*!< The URL base */
	pstd_type_model_t*   type_model;  /*!< The type model of this servlet */
	pstd_type_accessor_t path_acc;    /*!< The path accessor */
	pstd_type_accessor_t url_acc;     /*!< The output URL accessor */
	pstd_type_accessor_t method_acc;  /*!< The method accessor */
	uint32_t             GET_METHOD;  /*!< We only support GET */
} ctx_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxmem)
{
	if(argc < 2) 
		ERROR_RETURN_LOG(int, "Invalid servlet init param");

	ctx_t* ctx = (ctx_t*)ctxmem;

	memset(ctx, 0, sizeof(ctx_t));

	ctx->path = pipe_define("path", PIPE_INPUT, "plumber/std/request_local/String");
	ctx->request = pipe_define("request", PIPE_OUTPUT, "plumber/std_servlet/network/http/proxy/v0/Request");
	ctx->base = strdup(argv[1]);
	ctx->type_model = pstd_type_model_new();
	ctx->path_acc = pstd_type_model_get_accessor(ctx->type_model, ctx->path, "token");
	ctx->url_acc  = pstd_type_model_get_accessor(ctx->type_model, ctx->request, "url.token");
	ctx->method_acc = pstd_type_model_get_accessor(ctx->type_model, ctx->request, "method");

	PSTD_TYPE_MODEL_ADD_CONST(ctx->type_model, ctx->request, "GET", &ctx->GET_METHOD);

	return 0;
}

static int _unload(void* ctxmem)
{
	ctx_t* ctx = (ctx_t*)ctxmem;
	if(NULL != ctx->base) free(ctx->base);
	if(NULL != ctx->type_model) pstd_type_model_free(ctx->type_model);

	return 0;
}

static inline const char* _read_string(pstd_type_instance_t* type_inst, pstd_type_accessor_t acc)
{
	scope_token_t token = PSTD_TYPE_INST_READ_PRIMITIVE(scope_token_t, type_inst, acc);

	const pstd_string_t* obj = pstd_string_from_rls(token);

	return pstd_string_value(obj);
}

static int _exec(void* ctxmem)
{
	ctx_t* ctx = (ctx_t*)ctxmem;

	pstd_type_instance_t* inst = PSTD_TYPE_INSTANCE_LOCAL_NEW(ctx->type_model);

	const char* path = _read_string(inst, ctx->path_acc);

	pstd_string_t* url_obj = pstd_string_new(32);
	pstd_string_printf(url_obj, "%s%s", ctx->base, path);
	scope_token_t token = pstd_string_commit(url_obj);
	PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->url_acc, token);
	PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, ctx->method_acc, ctx->GET_METHOD);

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
