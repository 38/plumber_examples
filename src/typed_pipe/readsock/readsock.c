#include <stdlib.h>
#include <time.h>

#include <pservlet.h>
#include <pstd.h>
#include <proto.h>

typedef struct {
	pipe_t in;
	pipe_t out[3];

	uint32_t out0_sz;
	uint32_t out0_something_else;
	uint32_t out0_value;

	uint32_t out1_sz;
	uint32_t out1_timestamp;
	uint32_t out1_value;

	uint32_t out2_sz;
	uint32_t out2_timestamp;
	uint32_t out2_value;
	uint32_t out2_random;

} context_t;

typedef struct {
	int lineno;
	enum {
		DIGIT,
		REMAINING,
		LF
	} state;
	uint32_t values[3];
} state_t;

#define _CHK(what) if((what) == ERROR_CODE(typeof(what))) return ERROR_CODE(int);

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	ctx->in = pipe_define("in", PIPE_INPUT, NULL);
	ctx->out[0] = pipe_define("out0", PIPE_OUTPUT, "plumber_example/typed_pipe/InputValue1");
	ctx->out[1] = pipe_define("out1", PIPE_OUTPUT, "plumber_example/typed_pipe/InputValue2");
	ctx->out[2] = pipe_define("out2", PIPE_OUTPUT, "plumber_example/typed_pipe/InputValue3");

	proto_init();

	_CHK(ctx->out0_sz = proto_db_type_size("plumber_example/typed_pipe/InputValue1"));
	_CHK(ctx->out1_sz = proto_db_type_size("plumber_example/typed_pipe/InputValue2"));
	_CHK(ctx->out2_sz = proto_db_type_size("plumber_example/typed_pipe/InputValue3"));

	_CHK(ctx->out0_something_else = proto_db_type_offset("plumber_example/typed_pipe/InputValue1", "something_else", NULL));
	_CHK(ctx->out0_value = proto_db_type_offset("plumber_example/typed_pipe/InputValue1", "value", NULL));

	_CHK(ctx->out1_timestamp = proto_db_type_offset("plumber_example/typed_pipe/InputValue2", "timestamp", NULL));
	_CHK(ctx->out1_value = proto_db_type_offset("plumber_example/typed_pipe/InputValue2", "value", NULL));
	
	_CHK(ctx->out2_timestamp = proto_db_type_offset("plumber_example/typed_pipe/InputValue3", "timestamp", NULL));
	_CHK(ctx->out2_value = proto_db_type_offset("plumber_example/typed_pipe/InputValue3", "value", NULL));
	_CHK(ctx->out2_random = proto_db_type_offset("plumber_example/typed_pipe/InputValue3", "random", NULL));

	proto_finalize();

	return 0;
}

static int _process(char ch, pstd_dfa_process_param_t param)
{
	state_t* state = (state_t*)param.state;

	switch(state->state)
	{
		case DIGIT:
			if(ch >= '0' && ch <= '9')
				state->values[state->lineno] = state->values[state->lineno] * 10 + ch - '0';
			else if(ch == '\r')
				state->state = LF;
			else
				state->state = REMAINING;
			break;
		case REMAINING:
			if(ch == '\r')
				state->state = LF;
			break;
		case LF:
			if(ch == '\n')
			{
				state->state = DIGIT;
				state->lineno ++;
			}
			else
				state->state = REMAINING;
			break;
	}

	if(state->lineno == 3) 
		pstd_dfa_done(param.dfa);

	return 0;
}

static void* _create_state()
{
	state_t* state = NULL;
	state = malloc(sizeof(*state));
	state->lineno = 0;
	state->state  = DIGIT;
	state->values[0] = state->values[1] = state->values[2] = 0;
	return state;
}

static int _dispose_state(void* state)
{
	free(state);
	return 0;
}

static int _post_process(pstd_dfa_process_param_t param)
{

	context_t* ctx = (context_t*)param.data;
	state_t*   state = (state_t*)param.state;
	char buf0[ctx->out0_sz];
	char buf1[ctx->out1_sz];
	char buf2[ctx->out2_sz];

	*(uint32_t*)(buf0 + ctx->out0_something_else) = 123;
	*(uint32_t*)(buf0 + ctx->out0_value) = state->values[0];

	*(uint32_t*)(buf1 + ctx->out1_timestamp) = (uint32_t)time(NULL);
	*(uint32_t*)(buf1 + ctx->out1_value) = state->values[1];

	*(uint32_t*)(buf2 + ctx->out2_timestamp) = (uint32_t)time(NULL);
	*(uint32_t*)(buf2 + ctx->out2_value) = state->values[2];
	*(uint32_t*)(buf2 + ctx->out2_random) = rand();

	if(state->values[0]) pipe_hdr_write(ctx->out[0], buf0, ctx->out0_sz);
	if(state->values[1]) pipe_hdr_write(ctx->out[1], buf1, ctx->out1_sz);
	if(state->values[2]) pipe_hdr_write(ctx->out[2], buf2, ctx->out2_sz);

	return 0;
}

static int _exec(void* ctxbuf)
{
	context_t* ctx = (context_t*)ctxbuf;

	pstd_dfa_ops_t ops = {
		.create_state = _create_state,
		.dispose_state = _dispose_state,
		.process = _process,
		.post_process = _post_process
	};

	pstd_dfa_run(ctx->in, ops, ctx);
	return 0;
}

static inline int _cleanup(void* ctxbuf)
{
	(void)ctxbuf;

	return 0;
}

SERVLET_DEF = {
	.desc = "Read data from socket",
	.version = 0x0,
	.size = sizeof(context_t),
	.init = _init,
	.exec = _exec,
	.unload = _cleanup
};
