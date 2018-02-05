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
	pipe_t raw;
	pipe_t parsed;
	pipe_t error;

	pstd_type_model_t*   type_model;
	pstd_type_accessor_t url_acc;
	pstd_type_accessor_t method_acc;

	uint32_t GET_METHOD;

	char* url_prefix;
	uint32_t url_prefix_len;
} servlet_data_t;
int init(uint32_t argc, char const* const* argv, void* data)
{
	if(argc < 2) 
		ERROR_RETURN_LOG(int, "Invalid servlet init string, expected: %s <url_prefix>", argv[0]);

	servlet_data_t* sd = (servlet_data_t*)data;
	sd->raw    = pipe_define("raw", PIPE_INPUT, NULL);
	sd->parsed = pipe_define("parsed", PIPE_OUTPUT, "plumber/std_servlet/network/http/client/v0/Request");
	sd->error  = pipe_define("error", PIPE_OUTPUT, NULL);
	
	sd->type_model = pstd_type_model_new();
	sd->url_acc = pstd_type_model_get_accessor(sd->type_model, sd->parsed, "url.token");
	sd->method_acc = pstd_type_model_get_accessor(sd->type_model, sd->parsed, "method");
	PSTD_TYPE_MODEL_ADD_CONST(sd->type_model, sd->parsed, "GET", &sd->GET_METHOD);

	sd->url_prefix = strdup(argv[1]);
	sd->url_prefix_len = strlen(argv[1]);

	return 0;
}
typedef struct {
	enum {
		GET_G,
		GET_E,
		GET_T,
		GET_URL_WS,
		URL,
		GET_PARAM,
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
	size_t path_length;
} _state_t;
static inline _state_t* _state_new()
{
	_state_t* state = (_state_t*)malloc(sizeof(_state_t));
	state->ps = GET_G;
	state->keep_alive = 1;
	state->has_data = 0;
	state->field_name_length = state->field_value_length  = state->path_length = 0;
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
	char _buffer[4096];
	char* buffer = NULL;
	_state_t* state;
	size_t rem = 0, sz = 0;
	
	pipe_cntl(sd->raw, PIPE_CNTL_POP_STATE, &state);
	
	int new_state = 0;
	if(state == NULL)
	{
		state = _state_new();
		new_state = 1;
	}
	for(;state->ps != ERR && state->ps != OK;)
	{
		/* Before we actually started, we need to release the previously acquired internal buffer */
		size_t min_sz;
		if(buffer != _buffer && buffer != NULL && ERROR_CODE(int) == pipe_data_release_buf(sd->raw, buffer, sz))
			goto READ_ERR;

		sz = sizeof(_buffer);
		int rc = pipe_data_get_buf(sd->raw, sizeof(_buffer), (void const**)&buffer, &min_sz, &sz);
		if(ERROR_CODE(int) == rc) goto READ_ERR;

		if(rc == 0) 
		{
			buffer = _buffer;
			sz = pipe_read(sd->raw, buffer, sizeof(buffer));
		}
		
		if(sz == 0)
		{
			if(pipe_eof(sd->raw) > 0)
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
				pipe_cntl(sd->raw, PIPE_CNTL_SET_FLAG, PIPE_PERSIST);
				pipe_cntl(sd->raw, PIPE_CNTL_PUSH_STATE, state, _state_free);
				return 0;
			}
		}
		else if(sz == ERROR_CODE(size_t))
		{
READ_ERR:
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
				case URL: _FILL_BUFFER(ch == ' ' || ch == '\t' || ch == '?', path, GET_PARAM, ch);
				case GET_PARAM: _SKIP(ch == ' ' || ch == '\t'); state->ps = URL_VER_WS; 
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
				state->field_name_length = state->field_value_length = 0;
				state->field_name[0] = state->field_value[0] = 0;
			}

		}
		if(state->ps == OK)
		{
		    if(_buffer == buffer && rem + 1 < sz) 
				pipe_cntl(sd->raw, PIPE_CNTL_EOM, buffer, rem);
			
			if(_buffer != buffer)
				pipe_data_release_buf(sd->raw, buffer, rem);
		}
		if(state->ps == ERR)
		    state->keep_alive = 0;
	}

	if(state->keep_alive == 1)
	    pipe_cntl(sd->raw, PIPE_CNTL_SET_FLAG, PIPE_PERSIST);

	char itbuf[pstd_type_instance_size(sd->type_model)];
	pstd_type_instance_t* inst = pstd_type_instance_new(sd->type_model, itbuf);

	if(state->ps != OK)
	{
		pipe_write(sd->error, "400 Bad Request", 15);
	}
	else
	{
		LOG_NOTICE("Incoming HTTP Request: %s", state->path);
		pstd_string_t* url = pstd_string_new(state->path_length + sd->url_prefix_len + 1);
		pstd_string_write(url, sd->url_prefix, sd->url_prefix_len);
		pstd_string_write(url, state->path, state->path_length);
		scope_token_t st = pstd_string_commit(url);
		PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, sd->url_acc, st); 
		PSTD_TYPE_INST_WRITE_PRIMITIVE(inst, sd->method_acc, sd->GET_METHOD);
	}

	if(new_state == 1)
	    _state_free(state);

	pstd_type_instance_free(inst);


	return 0;
}

int unload(void* data)
{
	servlet_data_t* sd = (servlet_data_t*)data;
	free(sd->url_prefix);
	pstd_type_model_free(sd->type_model);
	return 0;
}
SERVLET_DEF = {
	.desc = "The servlet that parse a HTTP request and construct the reverse proxy request",
	.version = 0,
	.size = sizeof(servlet_data_t),
	.init = init,
	.exec = _exec,
	.unload = unload
};
