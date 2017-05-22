/**
 * Copyright (C) 2017, Hao Hou
 **/

#include <pservlet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pstd.h>
typedef struct {
	pipe_t mime;
	pipe_t size;
	pipe_t content;
	pipe_t error;
	pipe_t status;
	pipe_t response;
} data_t;

int init(uint32_t argc, char const* const* argv, void* d)
{
	(void) argc;
	(void) argv;
	data_t* data = (data_t*)d;
	data->mime = pipe_define("mime", PIPE_INPUT, NULL);
	data->content = pipe_define("content", PIPE_INPUT, NULL);
	data->error = pipe_define("error", PIPE_INPUT, NULL);
	data->size = pipe_define("size", PIPE_INPUT, NULL);
	data->status   = pipe_define("status", PIPE_INPUT, NULL);
	data->response = pipe_define("response", PIPE_OUTPUT | PIPE_ASYNC, NULL);

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
int exec(void* d)
{
	const char* status;
	const char* mime_type = "text/plain";


	char buffer[1024];
	data_t* data = (data_t*)d;
	if(!pipe_eof(data->error))
	{
		size_t sz = pipe_read(data->error, buffer, sizeof(buffer));
		buffer[sz] = 0;
		status = buffer;
	}
	else
	{
		size_t sz = pipe_read(data->status, buffer, sizeof(buffer));
		buffer[sz] = 0;
		status = buffer;
	}

	int use_token = 0;

	pstd_bio_t* out = pstd_bio_new(data->response);

	pstd_bio_printf(out, "HTTP/1.1 %s\r\n", status);

	if(status[0] != '2' && status[0] != '3') /* If this is not 2xx code or 3xx code*/
	{
		pstd_bio_printf(out, "Content-Type: text/html\r\n");
		const char* response = "<html><title>Ooops</title><body>Ooops, We can not find the page you request.</body></html>";
		pstd_bio_printf(out, "Content-Length: %zu\r\n", strlen(response));
		_connection_field(out, data->response);
		pstd_bio_printf(out, "Server: StaticFileServer/Plumber(%s)\r\n", runtime_version());
		pstd_bio_printf(out, "\r\n");
		pstd_bio_printf(out, "%s", response);

		pstd_bio_free(out);

		return 0;
	}
	else if(status[0] == '2') /* this is a 2xx code means we should use the mime type */
	{
		size_t sz = pipe_read(data->mime, buffer, sizeof(buffer));
		buffer[sz] = 0;
		mime_type = buffer;
		use_token = 1;
	}

	pstd_bio_printf(out, "Content-Type: %s\r\n", mime_type);
	_connection_field(out, data->response);
	pstd_bio_printf(out, "Server: PSS(%s)\r\n", runtime_version());

	size_t size;
	pipe_read(data->size, &size, sizeof(size));
	pstd_bio_printf(out, "Content-Length: %zu\r\n\r\n", size);

	if(use_token)
	{
		scope_token_t token;
		pipe_read(data->content, &token, sizeof(token));
		pstd_bio_write_scope_token(out, token);
	}
	else while(!pipe_eof(data->content))
	{
		size_t sz = pipe_read(data->content, buffer, sizeof(buffer));
		pstd_bio_write(out, buffer, sz);
	}

	pstd_bio_free(out);

	return 0;
}

int unload(void *data)
{
	(void) data;
	return 0;
}
SERVLET_DEF = {
	.desc = "Gerneate HTTP response based on the inputs",
	.version = 0,
	.size = sizeof(data_t),
	.init = init,
	.exec = exec,
	.unload = unload
};
