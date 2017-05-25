/**
 * Copyright (C) 2017, Hao Hou
 **/

#include <pservlet.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include <pstd.h>
#include <types/file.h>

typedef struct {
	const char* basedir;
	pipe_t path;
	pipe_t content;
	pipe_t status;
	pipe_t size;
	pipe_t error;
} data_t;
int init(uint32_t argc, char const* const* argv, void* d)
{
	if(argc != 2)
	{
		LOG_ERROR("Usage: %s <working-dir>", argv[0]);
		return -1;
	}

	data_t* data = (data_t*)d;

	data->basedir = argv[1];

	data->path = pipe_define("path", PIPE_INPUT, NULL);
	data->status = pipe_define("status", PIPE_OUTPUT, NULL);
	data->content = pipe_define("content", PIPE_OUTPUT, NULL);
	data->error = pipe_define("error", PIPE_OUTPUT, NULL);
	data->size = pipe_define("size", PIPE_OUTPUT, NULL);

	return 0;
}
int unload(void* d)
{
	(void) d;
	return 0;
}
static inline const char* _simplify_path(char* path)
{
	char*  bs[1024], * es[1024];
	char *begin = path, *end = path;
	int32_t sp = 0, i;
	for(;sp >= 0;end++)
	    if(*end == '/' || *end == 0 || *end == '?')
	    {
		    if(begin < end)
		    {
			    if(end - begin == 2 && begin[0] == '.' && begin[1] == '.') sp --;
			    else if(end - begin > 1 || begin[0] != '.') bs[sp] = begin, es[sp] = end, sp ++;
		    }
		    begin = end + 1;
		    if(*end == 0 || *end == '?') break;
	    }
	if(*end == '?') *end = 0;
	if(sp < 0) return NULL;
	begin = path;
	for(i = 0; i < sp; i ++)
	    memmove(begin, bs[i], (size_t)(es[i] - bs[i] + 1)), begin += es[i] - bs[i] + 1;
	*begin = 0;
	return path;
}
int exec(void* d)
{
	data_t* data = (data_t*)d;

	char relative_path[1024];

	if(pipe_eof(data->path) > 0)
	{
		return 0;
	}

	size_t sz = pipe_read(data->path, relative_path, sizeof(relative_path));
	relative_path[sz] = 0;
	if(_simplify_path(relative_path) == NULL)
	{
		pipe_write(data->error, "404 Not Found", 13);
		return 0;
	}

	char abs_path[1024];
	snprintf(abs_path, sizeof(abs_path), "%s/%s", data->basedir, relative_path);

	struct stat s;
	if(pstd_fcache_stat(abs_path, &s) == ERROR_CODE(int))
	{
		pipe_write(data->error, "404 Not Found", 13);
		return 0;
	}

	FILE* fp = NULL;

	char buffer[1024];
	size_t total = 0;


	if(!(s.st_mode & S_IFREG))
	{
		char index[1024];
		char path[1024];
		snprintf(index, sizeof(index), "%s/index.html", relative_path);
		_simplify_path(index);
		snprintf(path, sizeof(path), "%s/%s", data->basedir, index);
		if((s.st_mode & S_IFDIR) && access(path, R_OK) == 0)
		{
			pipe_write(data->status, "302 Found\r\n", 11);
			snprintf(buffer, sizeof(buffer), "Location: /%s", index);
			pipe_write(data->status, buffer, strlen(buffer));
			snprintf(buffer, sizeof(buffer), "The document has been moved permanently to %s.", index);
			total = strlen(buffer);
			pipe_write(data->content, buffer, total);
		}
		else
		{
			pipe_write(data->error, "404 Not Found", 13);
			return 0;
		}
	}
	else
	{

		pstd_file_t* file = pstd_file_new(abs_path);
		if(pstd_file_exist(file) != 1)
		{
			pipe_write(data->error, "404 Not Found", 13);
			return 0;
		}
		pipe_write(data->status, "200 OK", 9);
		scope_token_t token = pstd_file_commit(file);
		pipe_write(data->content, &token, sizeof(token));
		total = pstd_file_size(file);
	}

	pipe_write(data->size, &total, sizeof(total));

	if(NULL != fp) fclose(fp);

	return 0;
}

SERVLET_DEF = {
	.desc = "Read a file from disk",
	.version = 0,
	.size = sizeof(data_t),
	.init = init,
	.exec = exec,
	.unload = unload
};
