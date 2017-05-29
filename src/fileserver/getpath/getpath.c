/**
 * Copyright (C) 2017, Hao Hou
 **/
#include <pservlet.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <module/tls/api.h>

#include <pstd/type.h>
#include <pstd/types/string.h>
typedef struct {
	pipe_t request;
	pipe_t path;
	pipe_t host;
	pipe_t error;

	pstd_type_model_t*   type_model;
	pstd_type_accessor_t path_token;
	pstd_type_accessor_t host_token;
} servlet_data_t;
int init(uint32_t argc, char const* const* argv, void* data)
{
	(void) argc;
	(void) argv;
	servlet_data_t* sd = (servlet_data_t*)data;
	sd->request = pipe_define("request", PIPE_INPUT, NULL);
	sd->path    = pipe_define("path", PIPE_OUTPUT, "plumber/std/request_local/String");
	sd->error   = pipe_define("error", PIPE_OUTPUT, NULL);
	sd->host    = pipe_define("host", PIPE_OUTPUT, "plumber/std/request_local/String");

	if(sd->request == (pipe_t)-1 || sd->path == (pipe_t)-1 || sd->error == (pipe_t)-1)
	{
		LOG_ERROR("Cannot initialize the servlet");
		return -1;
	}

	sd->type_model = pstd_type_model_new();
	sd->path_token = pstd_type_model_get_accessor(sd->type_model, sd->path, "token");
	sd->host_token = pstd_type_model_get_accessor(sd->type_model, sd->host, "token");

	return 0;
}
typedef struct {
	enum {
		GET_G,
		GET_E,
		GET_T,
		GET_URL_WS,
		URL,
		URL_VER_WS,
		VER,
		VER_CR,
		VER_LF,
		FIELD,
		FIELD_NAME,
		FIELD_NAME_VALUE_SEP,
		FIELD_NAME_VALUE_WS,
		FIELD_VLAUE,
		FIELD_CR,
		FIELD_LF,
		//	REQUEST_CR,
		REQUEST_LF,
		OK,
		ERR
	} ps;                  /* parser state */
	char field_name[16];   /* currently we only care about connection */
	size_t field_name_length;
	char field_value[32];  /* only closed and keep-aliave */
	size_t field_value_length;
	int keep_alive;
	int has_data;
	char path[1024];
	char host[512];
	size_t path_length;
} _state_t;
static inline _state_t* _state_new()
{
	_state_t* state = (_state_t*)malloc(sizeof(_state_t));
	state->ps = GET_G;
	state->keep_alive = 1;
	state->has_data = 0;
	state->field_name_length = state->field_value_length  = state->path_length = 0;
	state->host[0] = 0;
	return state;
}

static inline int _state_free(_state_t* state)
{
	free(state);
	return 0;
}

int _exec(void* data)
{
	servlet_data_t* sd = (servlet_data_t*)data;
	char buffer[4096];
	_state_t* state;
	size_t rem = 0, sz = 0;
	pipe_cntl(sd->request, PIPE_CNTL_POP_STATE, &state);
	/*
	 * Example: Call the pipe specified control function
	 * pipe_cntl(sd->request, MODULE_TLS_CNTL_ENCRYPTION, 0);
	 **/
	int new_state = 0;
	if(state == NULL)
	{
		state = _state_new();
		new_state = 1;
	}
	for(;state->ps != ERR && state->ps != OK;)
	{
		sz = pipe_read(sd->request, buffer, sizeof(buffer));
		if(sz == 0)
		{
			if(pipe_eof(sd->request) > 0)
			{
				state->keep_alive = 0;
				if(!state->has_data)
				{
					/* If the request doesn't contains any data, it means the connection is already closed */
					if(new_state) _state_free(state);
					return 0;
				}
				break;
			}
			else
			{
				pipe_cntl(sd->request, PIPE_CNTL_SET_FLAG, PIPE_PERSIST); /* because we do not want the framework close this connection */
				pipe_cntl(sd->request, PIPE_CNTL_PUSH_STATE, state, _state_free);
				return 0;
			}
		}
		else if(sz == ERROR_CODE(size_t))
		{
			if(new_state) _state_free(state);
			return 0;
		}
#define _EXPECT(chr, next) \
		if(ch == (chr)) state->ps = next;\
		else state->ps = ERR;\
		break
#define _WS(next) \
		if(ch != ' ' && ch != '\t') \
		    state->ps = next;\
		else break;
#define _FILL_BUFFER(until, buf, next, exp)\
		if(!(until))  \
		{\
			if(state->buf##_length < sizeof(state->buf)) \
			    state->buf[state->buf##_length++] = exp;\
			break;\
		}\
		else \
		{\
			state->buf[state->buf##_length] = 0;\
			state->ps = next;\
		}
#define _SKIP(until) \
		if(!(until)) break
		state->has_data = 1;
		for(rem = 0; rem < sz && state->ps != ERR && state->ps != OK; rem ++)
		{
			char ch = buffer[rem];
			switch(state->ps)
			{
				case GET_G: _EXPECT('G', GET_E);
				case GET_E: _EXPECT('E', GET_T);
				case GET_T: _EXPECT('T', GET_URL_WS);
				case GET_URL_WS: _WS(URL);
				case URL: _FILL_BUFFER(ch == ' ' || ch == '\t', path, URL_VER_WS, ch);
				case URL_VER_WS: _WS(VER);
				case VER: _SKIP(ch == '\r');
				case VER_CR: _EXPECT('\r', VER_LF);
				case VER_LF: _EXPECT('\n', FIELD);
				case FIELD:
				     if(ch == '\r')
				     {
					     state->ps = REQUEST_LF;
					     break;
				     }
				     else state->ps = FIELD_NAME;
				case FIELD_NAME:
				     if(ch < 32)
				     {
					     state->ps = ERR;
					     break;
				     }
				     _FILL_BUFFER(ch == ':', field_name, FIELD_NAME_VALUE_SEP, (ch | (0x20 & (ch >> 1))));
				case FIELD_NAME_VALUE_SEP: _EXPECT(':', FIELD_NAME_VALUE_WS);
				case FIELD_NAME_VALUE_WS: _WS(FIELD_VLAUE);
				case FIELD_VLAUE: _FILL_BUFFER(ch == '\r', field_value, FIELD_CR, (ch | (0x20 & (ch >> 1))));
				case FIELD_CR: _EXPECT('\r', FIELD_LF);
				case FIELD_LF: _EXPECT('\n', FIELD);
				case REQUEST_LF: _EXPECT('\n', OK);
				default: LOG_ERROR("Bug!");
			}
			if(state->ps == FIELD_LF)
			{
				if(strcmp(state->field_name, "connection") == 0  && strcmp(state->field_value, "keep-alive") == 0)
				    state->keep_alive = 1;
				if(strcmp(state->field_name, "host") == 0)
				    strcpy(state->host, state->field_value);
				state->field_name_length = state->field_value_length = 0;
				state->field_name[0] = state->field_value[0] = 0;
			}

		}
		if(state->ps == OK && rem + 1 < sz)
		    pipe_cntl(sd->request, PIPE_CNTL_EOM, buffer, rem);
		if(state->ps == ERR)
		    state->keep_alive = 0;
	}

	if(state->keep_alive == 1)
	    pipe_cntl(sd->request, PIPE_CNTL_SET_FLAG, PIPE_PERSIST);


	char itbuf[pstd_type_instance_size(sd->type_model)];
	pstd_type_instance_t* inst = pstd_type_instance_new(sd->type_model, itbuf);

	if(state->ps != OK)
	{
		pipe_write(sd->error, "400 Bad Request", 15);
	}
	else
	{
		LOG_NOTICE("Incoming HTTP Request: %s", state->path);
		//pipe_write(sd->path, state->path, state->path_length);
		pstd_string_t* pstr = pstd_string_new(state->path_length + 1);
		pstd_string_write(pstr, state->path, state->path_length);
		scope_token_t st = pstd_string_commit(pstr);
		PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, sd->path_token, st); 
	}

	if(state->host[0])
	{
	    //pipe_write(sd->host, state->host, strlen(state->host));
		pstd_string_t* pstr = pstd_string_new(strlen(state->host) + 1);
		pstd_string_write(pstr, state->host, strlen(state->host));
		scope_token_t st = pstd_string_commit(pstr);
		PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, sd->host_token, st); 
	}

	if(new_state == 1)
	    _state_free(state);

	pstd_type_instance_free(inst);

	return 0;
}

int unload(void* data)
{
	servlet_data_t* sd = (servlet_data_t*)data;
	pstd_type_model_free(sd->type_model);
	return 0;
}
SERVLET_DEF = {
	.desc = "Servlet to get path from the HTTP Request",
	.version = 0,
	.size = sizeof(servlet_data_t),
	.init = init,
	.exec = _exec,
	.unload = unload
};
