#include <stdlib.h>
#include <time.h>

#include <pservlet.h>
#include <pstd.h>

typedef struct {
	int answer;         // The answer this game
	int times_guessed;  // How many times you have guessed
	pipe_t   input, output;
} private_context_t;

static int _init(uint32_t argc, char const* const* argv, void* ctxbuf)
{
	private_context_t* context = (private_context_t*)ctxbuf;

	srand((unsigned)time(NULL));

	context->answer = (rand() % 100);
	context->times_guessed = 0;

	context->input = pipe_define("input", PIPE_INPUT, NULL);
	context->output = pipe_define("output", PIPE_OUTPUT, NULL);

	return 0;
}

static int _exec(void* ctxbuf)
{
	private_context_t* context = (private_context_t*)ctxbuf;

	pstd_bio_t* in  = pstd_bio_new(context->input);
	pstd_bio_t* out = pstd_bio_new(context->output);

	char buf[128];
	size_t bytes_read = pstd_bio_read(in, buf, sizeof(buf) - 1);
	buf[bytes_read] = 0;
	int guessed = atoi(buf);

	context->times_guessed ++;
	if(guessed < context->answer)
		pstd_bio_printf(out, "Round %d: too small\n", context->times_guessed);
	else if(guessed > context->answer)
		pstd_bio_printf(out, "Round %d: too large\n", context->times_guessed);
	else
		pstd_bio_printf(out, "Round %d: You win!\n", context->times_guessed);


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
