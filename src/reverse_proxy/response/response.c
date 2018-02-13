/**
 * Copyright (C) 2017, Hao Hou
 **/

#include <pservlet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pstd.h>

#include <pstd/types/string.h>
#include <pstd/types/file.h>

typedef struct {
	pipe_t    response;
	pipe_t    output;
	pipe_t    bad_request;
	pipe_t    internal_err;
	pstd_type_model_t*    type_model;
	pstd_type_accessor_t  token_acc;
#if 0
	pstd_type_accessor_t  status_acc;
	pstd_type_accessor_t  body_acc;
	pstd_type_accessor_t  header_acc;
#endif
} context_t;

int init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	ctx->response = pipe_define("response", PIPE_INPUT, "plumber/std_servlet/network/http/proxy/v0/Response");
	ctx->output = pipe_define("output", PIPE_OUTPUT | PIPE_ASYNC, NULL);
	ctx->internal_err = pipe_define("500", PIPE_INPUT, NULL);
	ctx->bad_request = pipe_define("400", PIPE_INPUT, NULL);

	ctx->type_model = pstd_type_model_new();
#if 0
	ctx->status_acc = pstd_type_model_get_accessor(ctx->type_model, ctx->response, "status");
	ctx->body_acc   = pstd_type_model_get_accessor(ctx->type_model, ctx->response, "body.token");
	ctx->header_acc = pstd_type_model_get_accessor(ctx->type_model, ctx->response, "header.token");
#endif
	ctx->token_acc = pstd_type_model_get_accessor(ctx->type_model, ctx->response, "token");

	return 0;
}

static inline int _write_token(pstd_bio_t* bio, pstd_type_instance_t* inst, pstd_type_accessor_t acc)
{
	scope_token_t scope = PSTD_TYPE_INST_READ_PRIMITIVE(scope_token_t, inst, acc);
	return pstd_bio_write_scope_token(bio, scope);
}

int _exec(void* d)
{
	context_t* ctx = (context_t*)d;
	pstd_bio_t* out = pstd_bio_new(ctx->output);

	size_t sz = pstd_type_instance_size(ctx->type_model);
	char ibuf[sz];
	pstd_type_instance_t* inst = pstd_type_instance_new(ctx->type_model, ibuf);

	if(pipe_eof(ctx->internal_err) == 0)
	{
		static const char response[] = "<html><title>Internal Error</title><body>Ooops, we have an internal error on our server.</body></html>";
		pstd_bio_printf(out, "HTTP/1.1 500 Internal Error\r\n");
		pstd_bio_printf(out, "Content-Type: text/html\r\n");
	    pstd_bio_printf(out, "Connection: close\r\n");
		pstd_bio_printf(out, "Content-Length: %zu\r\n\r\n", sizeof(response) - 1);
		pstd_bio_printf(out, "%s", response);
		pipe_cntl(ctx->output, PIPE_CNTL_CLR_FLAG, PIPE_PERSIST);
		goto RET;
	}
	
	if(pipe_eof(ctx->bad_request) == 0)
	{
		static const char response[] = "<html><title>Bad Request</title><body>We can't understand what you said, sorry. </body></html>";
		pstd_bio_printf(out, "HTTP/1.1 400 Bad Request\r\n");
		pstd_bio_printf(out, "Content-Type: text/html\r\n");
	    pstd_bio_printf(out, "Connection: close\r\n");
		pstd_bio_printf(out, "Content-Length: %zu\r\n\r\n", sizeof(response) - 1);
		pstd_bio_printf(out, "%s", response);
		pipe_cntl(ctx->output, PIPE_CNTL_CLR_FLAG, PIPE_PERSIST);
		goto RET;
	}

	_write_token(out, inst, ctx->token_acc);


RET:
	pstd_bio_free(out);
	pstd_type_instance_free(inst);

	return 0;
}

int unload(void *data)
{
	context_t* ctx = (context_t*)data;
	pstd_type_model_free(ctx->type_model);
	return 0;
}
SERVLET_DEF = {
	.desc = "Gerneate HTTP response based on the inputs",
	.version = 0,
	.size = sizeof(context_t),
	.init = init,
	.exec = _exec,
	.unload = unload
};
