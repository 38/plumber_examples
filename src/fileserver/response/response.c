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
	pipe_t    file;
	pipe_t    mime;
	pipe_t    output;
	pipe_t    bad_request;
	pipe_t    interal_err;
	pipe_t    invalid_path;
	pstd_type_model_t*    type_model;
	pstd_type_accessor_t  status_acc;
	pstd_type_accessor_t  file_acc;
	pstd_type_accessor_t  redir_acc;
	pstd_type_accessor_t  mime_acc;
} context_t;

int init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	ctx->file   = pipe_define("file", PIPE_INPUT, "plumber/std_servlet/filesystem/readfile/v0/Result");
	ctx->mime   = pipe_define("mime", PIPE_INPUT, "plumber/std/request_local/String");
	ctx->output = pipe_define("output", PIPE_OUTPUT | PIPE_ASYNC, NULL);
	ctx->interal_err = pipe_define("500", PIPE_INPUT, NULL);
	ctx->bad_request = pipe_define("400", PIPE_INPUT, NULL);
	ctx->invalid_path = pipe_define("403", PIPE_INPUT, NULL);

	ctx->type_model = pstd_type_model_new();
	ctx->status_acc = pstd_type_model_get_accessor(ctx->type_model, ctx->file, "status");
	ctx->file_acc   = pstd_type_model_get_accessor(ctx->type_model, ctx->file, "file.token");
	ctx->redir_acc  = pstd_type_model_get_accessor(ctx->type_model, ctx->file, "redirect.token");
	ctx->mime_acc   = pstd_type_model_get_accessor(ctx->type_model, ctx->mime, "token");

	return 0;
}

static inline void _connection_field(pstd_bio_t* out, pipe_t res)
{
	pipe_flags_t flags;
	pipe_cntl(res, PIPE_CNTL_GET_FLAGS, &flags);
	if(flags & PIPE_PERSIST)
	    pstd_bio_printf(out, "Connection: keep-alive\r\n");
	else
	    pstd_bio_printf(out, "Connection: close\r\n");
}

static inline const char* _read_str(pstd_type_instance_t* inst, pstd_type_accessor_t acc)
{
	scope_token_t scope = PSTD_TYPE_INST_READ_PRIMITIVE(scope_token_t, inst, acc);
	const pstd_string_t* pstr = pstd_string_from_rls(scope);
	return pstd_string_value(pstr);
}

int _exec(void* d)
{
	context_t* ctx = (context_t*)d;
	pstd_bio_t* out = pstd_bio_new(ctx->output);

	size_t sz = pstd_type_instance_size(ctx->type_model);
	char ibuf[sz];
	pstd_type_instance_t* inst = pstd_type_instance_new(ctx->type_model, ibuf);

	if(pipe_eof(ctx->interal_err) == 0)
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

	if(pipe_eof(ctx->invalid_path) == 0)
	{
		static const char response[] = "<html><title>Forbidden</title><body>Sorry, we are not allowed to find what you want.</body></html>";
		pstd_bio_printf(out, "HTTP/1.1 403 Forbidden\r\n");
		pstd_bio_printf(out, "Content-Type: text/html\r\n");
	    pstd_bio_printf(out, "Connection: close\r\n");
		pstd_bio_printf(out, "Content-Length: %zu\r\n\r\n", sizeof(response) - 1);
		pstd_bio_printf(out, "%s", response);
		pipe_cntl(ctx->output, PIPE_CNTL_CLR_FLAG, PIPE_PERSIST);
		goto RET;
	}

	uint32_t status = PSTD_TYPE_INST_READ_PRIMITIVE(uint32_t, inst, ctx->status_acc);
	if(status == 200)
	{
		scope_token_t scope = PSTD_TYPE_INST_READ_PRIMITIVE(scope_token_t, inst, ctx->file_acc);
		const pstd_file_t* file = pstd_file_from_rls(scope);

		pstd_bio_printf(out, "HTTP/1.1 200 OK\r\n");
		pstd_bio_printf(out, "Content-Type: %s\r\n", _read_str(inst, ctx->mime_acc));
		pstd_bio_printf(out, "Content-Length: %zu\r\n", pstd_file_size(file));
		_connection_field(out, ctx->output);
		pstd_bio_printf(out, "Server: StaticFileServer/Plumber(%s)\r\n\r\n", runtime_version());
		pstd_bio_write_scope_token(out, scope);
	}
	else if(status == 302)
	{
		static const char response[] = "<html><title>Page Moved</title><body>The page has been moved.</body></html>";
		pstd_bio_printf(out, "HTTP/1.1 302 Move Permenantly\r\n");
		pstd_bio_printf(out, "Content-Type: text/plain\r\n");
		pstd_bio_printf(out, "Content-Length: %zu\r\n", sizeof(response) - 1);
		pstd_bio_printf(out, "Location: %s\r\n", _read_str(inst, ctx->redir_acc));
		_connection_field(out, ctx->output);
		pstd_bio_printf(out, "Server: StaticFileServer/Plumber(%s)\r\n\r\n%s", runtime_version(),response);
	}
	else if(status == 404)
	{
		static const char response[] = "<html><title>Ooops</title><body>Ooops, We can not find the page you request.</body></html>";
		pstd_bio_printf(out, "HTTP/1.1 404 Not Found\r\n");
		pstd_bio_printf(out, "Content-Type: text/html\r\n");
		pstd_bio_printf(out, "Content-Length: %zu\r\n", sizeof(response) - 1);
		_connection_field(out, ctx->output);
		pstd_bio_printf(out, "Server: StaticFileServer/Plumber(%s)\r\n\r\n", runtime_version());
		pstd_bio_printf(out, "%s", response);
	}

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
