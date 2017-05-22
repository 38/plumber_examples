/**
 * Copyright (C) 2017, Hao Hou
 **/

#include <pservlet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Based on the mime.types file, which you can find from https://svn.apache.org/repos/asf/httpd/httpd/trunk/docs/conf/mime.types */
typedef struct {
	size_t cap;
	size_t num;
	char** data;
	pipe_t path;
	pipe_t mime;
} data_t;

int init(uint32_t argc, char const* const* argv, void* d)
{
	data_t* data = (data_t*)d;

	if(argc != 2)
	{
		LOG_ERROR("Usage: %s <mime.types file>", argv[0]);
		return -1;
	}
	FILE* fp = fopen(argv[1], "r");
	if(NULL == fp)
	{
		LOG_ERROR("cannot open mime.types file");
		return -1;
	}
	data->cap = 128;
	data->num = 0;
	data->data = (char**)calloc(data->cap, sizeof(char*));
	if(NULL == data->data)
	{
		LOG_ERROR("cannot allocate memory");
		return -1;
	}
	char buffer[1024];
	while(!feof(fp))
	{
		if(NULL == fgets(buffer, sizeof(buffer), fp)) break;
		if(buffer[0] == '#') continue;
		char *ext[32], *ptr;
		size_t num_ext = 0, size = 0, i;
		int state = 0;
		//sscanf(buffer, "%s%s", mime, ext);
		for(ptr = buffer; *ptr; ptr ++)
		{
			if(state == 0 && *ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n')
			{
				if(num_ext >= sizeof(ext)/ sizeof(*ext)) break;
				ext[num_ext++] = ptr;
				state = 1;
			}
			else if(state == 1)
			{
				if(*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n')
				{
					*ptr = 0;
					state = 0;
				}
				size ++;
			}
		}
		if(state == 1) size ++;

		while(data->cap <= data->num + num_ext - 1)
		{
			data->data = (char**)realloc(data->data, data->cap * sizeof(char*) * 2);
			data->cap *= 2;
		}

		for(i = 1; i < num_ext; i ++)
		{
			data->data[data->num] = (char*)malloc(strlen(ext[0]) + 1 + strlen(ext[i]) + 1);
			memcpy(data->data[data->num], ext[i], strlen(ext[i]) + 1);
			memcpy(data->data[data->num] + strlen(ext[i]) + 1, ext[0], strlen(ext[0]) + 1);
			data->num ++;
		}
	}
	LOG_DEBUG("%zu entry has been loaded from file %s", data->num, argv[1]);
	data->path = pipe_define("path", PIPE_INPUT, NULL);
	data->mime = pipe_define("mime", PIPE_OUTPUT, NULL);
	fclose(fp);
	return 0;
}

int unload(void* data)
{
	data_t* d = (data_t*)data;
	size_t i;
	for(i = 0; i < d->num; i ++)
	    free(d->data[i]);
	free(d->data);
	return 0;
}

int exec(void* data)
{
	data_t* d = (data_t*)data;
	char buffer[1024];
	buffer[0] = 0;
	size_t sz = 0;
	while(!pipe_eof(d->path))
	    sz = pipe_read(d->path, buffer, sizeof(buffer));
	buffer[sz] = 0;
	char* ptr = buffer, *ext = NULL;
	for(; *ptr; ptr ++)
	    if(*ptr == '.') ext = ptr + 1;
	    else if(*ptr == '?')
	    {
		    *ptr = 0;
		    break;
	    }

	size_t i;
	if(NULL != ext)
	{
		for(i = 0; i < d->num; i ++)
		{
			if(strcmp(d->data[i], ext) == 0)
			{
				const char* mime = d->data[i] + strlen(d->data[i]) + 1;
				pipe_write(d->mime, mime, strlen(mime));
				return 0;
			}
		}
	}

	pipe_write(d->mime, "application/octet-stream", strlen("application/octet-stream"));

	return 0;
}


SERVLET_DEF = {
	.desc = "Guess the MIME type from the file name",
	.version  = 0,
	.size = sizeof(data_t),
	.init = init,
	.exec = exec,
	.unload = unload
};
