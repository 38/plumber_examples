#include <stdlib.h>
#include <time.h>

#include <pservlet.h>
#include <pstd.h>

typedef struct {
	uint32_t upper_range;
	pipe_t   input, output;
} private_context_t;

typedef struct {
	int answer;         // The answer this game
	int times_guessed;  // How many times you have guessed
} game_session_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	private_context_t* context = (private_context_t*)ctxbuf;
	srand((unsigned)time(NULL));
	context->input = pipe_define("input", PIPE_INPUT | PIPE_PERSIST, NULL);
	context->output = pipe_define("output", PIPE_OUTPUT, NULL);

	context->upper_range = argc > 1 ? atoi(argv[1]) : 100;

	return 0;
}

static game_session_t* _create_session(private_context_t* ctx)
{
	game_session_t* ret = (game_session_t*)malloc(sizeof(game_session_t));

	ret->answer = (rand() % ctx->upper_range);
	ret->times_guessed = 0;

	LOG_ERROR("The answer is %d", ret->answer);

	return ret;
}

static int _destory_session(void* session)
{
	free(session);
	return 0;
}

static int _exec(void* ctxbuf)
{
	private_context_t* context = (private_context_t*)ctxbuf;

	pstd_bio_t* in  = pstd_bio_new(context->input);
	pstd_bio_t* out = pstd_bio_new(context->output);

	game_session_t* cur_session = NULL;

	// The very first thing we should do is retrieve current game session attached
	// to the pipe resource
	pipe_cntl(context->input, PIPE_CNTL_POP_STATE, &cur_session);

	// If there's no session attached, we just get a new session, so create a new game session
	// for it
	if(NULL == cur_session)
		cur_session = _create_session(context);

	// Read the user input
	char buf[128];
	size_t bytes_read = pstd_bio_read(in, buf, sizeof(buf) - 1);
	buf[bytes_read] = 0;
	int guessed = atoi(buf);

	// The actual game logic
	cur_session->times_guessed ++;

	if(guessed == cur_session->answer)
	{
		// Unlike the previous example, we want to close the TCP connection at this time
		pstd_bio_printf(out, "Round %d: You win!\n", cur_session->times_guessed);

		// A this point, we should close the game session
		// To make this happen, we only need to remove the persist flag
		pipe_cntl(context->input, PIPE_CNTL_CLR_FLAG, PIPE_PERSIST);

		// If we haven't push the state to the system, we need to dispose it manually
		if(cur_session->times_guessed == 1)
			_destory_session(cur_session);

		goto EXIT;
	}
	else if(guessed < cur_session->answer)
		pstd_bio_printf(out, "Round %d: too small\n", cur_session->times_guessed);
	else if(guessed > cur_session->answer)
		pstd_bio_printf(out, "Round %d: too large\n", cur_session->times_guessed);

	// If we need to continue the game, we need to re-attach the modified session to the pipe resource
	pipe_cntl(context->input, PIPE_CNTL_PUSH_STATE, cur_session, _destory_session);

EXIT:
	pstd_bio_free(in);
	pstd_bio_free(out);

	return 0;
}

SERVLET_DEF = {
	.desc = "The number guessing game implementation",
	.size = sizeof(private_context_t),
	.init = _init,
	.exec = _exec
};
