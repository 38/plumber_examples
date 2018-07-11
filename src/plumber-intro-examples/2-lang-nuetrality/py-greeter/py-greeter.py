from pservlet import *
def init(args):
    return (pipe_define("input", PIPE_INPUT),
            pipe_define("output", PIPE_OUTPUT))
def execute(ctx):
    name = pipe_read(ctx[0])
    pipe_write(ctx[1], name + "hello from Python\n")
    return 0
