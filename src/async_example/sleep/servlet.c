/**
 * Copyright (C) 2017, Hao Hou
 **/
#include <stdlib.h>
#include <pservlet.h>
#include <pstd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include <unistd.h>
/**
 * @brief The servlet context
 **/
typedef struct {
	enum {
		TYPE_SYNC,            /*!< This sleep servlet uses the sync function */
		TYPE_ASYNC,           /*!< This sleep servlet uses the async function */
		TYPE_ASYNC_WAIT       /*!< This sleep servlet uses the async wait mode */
	}               type;     /*!< The operating mode for this servlet */
	pipe_t          input;    /*!< The input pipe */
	pipe_t          output;   /*!< The output pipes */
} context_t;

typedef struct {
	int wait;         /*!< If this is the wait mode */
	pthread_t thread; /*!< The wait thread */
} async_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	if(argc != 2)
	    ERROR_RETURN_LOG(int, "Usage: %s <sync|async|async_wait>", argv[0]);

	const char* mode = argv[1];
	
	if(strcmp(mode, "sync") == 0) ctx->type = TYPE_SYNC;
	if(strcmp(mode, "async") == 0) ctx->type = TYPE_ASYNC;
	if(strcmp(mode, "async_wait") == 0) ctx->type = TYPE_ASYNC_WAIT;

	if(ERROR_CODE(pipe_t) == (ctx->input = pipe_define("in", PIPE_INPUT, "$T")))
	    ERROR_RETURN_LOG(int, "Cannot define the input pipe");

	if(ERROR_CODE(pipe_t) == (ctx->output = pipe_define("out", PIPE_MAKE_SHADOW(ctx->input), "$T")))
		ERROR_RETURN_LOG(int, "Cannot create output pipe");

	return ctx->type == TYPE_SYNC ? 0 : 1;
}

static int _unload(void* ctxbuf)
{
	(void)ctxbuf;
	return 0;
}

static int _async_init(async_handle_t* handle, void* buf, void* ctxbuf)
{
	(void)handle;
	context_t* ctx = (context_t*)ctxbuf;
	async_t* async_buf = (async_t*)buf;
	async_buf->wait = (ctx->type == TYPE_ASYNC_WAIT);
	async_cntl(handle, ASYNC_CNTL_CANCEL, 0);
	LOG_DEBUG("async init called");
	return 0;
}

static void* _do_sleep(void* args)
{
	async_handle_t* handle = (async_handle_t*)args;
	usleep(20000000);
	async_cntl(handle, ASYNC_CNTL_NOTIFY_WAIT, 0);
	return 0;
}

static int _async_exec(async_handle_t* handle, void* buf)
{
	LOG_DEBUG("async exec called");
	async_t* data = (async_t*)buf;
	if(!data->wait)
	{
		usleep(20000000);
	}
	else
	{
		if(ERROR_CODE(int) == async_cntl((async_handle_t*)handle, ASYNC_CNTL_SET_WAIT))
			ERROR_RETURN_LOG(int, "Cannot set the async task to wait mode");
		if(pthread_create(&data->thread, NULL, _do_sleep, (void*)handle) < 0)
		{
			async_cntl(handle, ASYNC_CNTL_NOTIFY_WAIT, ERROR_CODE(int));
			ERROR_RETURN_LOG_ERRNO(int, "Cannot start the actual wait thread");
		}
	}
	return 0;
}

static int _async_cleanup(async_handle_t* handle, void* buf, void* ctxbuf)
{
	(void)handle;
	context_t* ctx = (context_t*)ctxbuf;
	async_t*   data = (async_t*)buf;
	LOG_DEBUG("async cleanup called");

	if(ctx->type == TYPE_ASYNC_WAIT)
		pthread_join(data->thread, NULL);

	
	return 0;
}

static inline int _sync_exec(void* buf)
{
	(void)buf;
	usleep(20000000);
	return 0;
}

SERVLET_DEF = {
	.desc = "The duplicator duplicates a single input to multiple output",
	.version = 0x0,
	.size = sizeof(context_t),
	.async_buf_size = sizeof(int),
	.init = _init,
	.unload = _unload,
	.async_setup = _async_init,
	.async_exec  = _async_exec,
	.async_cleanup = _async_cleanup,
	.exec = _sync_exec
};
