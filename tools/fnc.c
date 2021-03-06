// New BSD License, part of efene, see LICENSE for details
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fnc.h"

#define STR_BUFFER_SIZE 2048
#define PATHS_SIZE 512

// copy the path to efene from the environment variable FNPATH if available
// if not found return NULL
// you have to free the variable
char* get_efene_path_from_env() {
	char *fnpath, *result;

	result = NULL;
	fnpath = getenv("FNPATH");

	if (fnpath != NULL) {
		result = strdup(fnpath);
		assert(result != NULL);
	}

	return result;
}

void show_usage() {
	printf("usage:\n");
	printf("\tfnc -s: run the interactive shell\n");
	printf("\tfnc -h: shows this help\n");
	printf("\tfnc -r module function [argumens*]: runs function from module passing optional arguments\n");
	printf("options:\n");
	printf("\t-t: type, can be beam (the default), lex, tree, ast, mod, erl or erl2ast\n");
	printf("\t-o: output path, the path where the compiled files will be written\n");
	printf("\t-T: test, instead of running the command print it to the screen\n");
	printf("\t-a: append a path to look for modules\n");
	printf("\t-p: prepend a path to look for modules\n");
	printf("examples:\n");
	printf("\tfnc foo.ifn: compile foo.ifn, write the result in the current directory\n");
	printf("\tfnc foo.ifn bar.fn baz.ifn: same as before but multiple files compiled\n");
	printf("\tfnc foo.ifn -o /tmp: compile foo.ifn, write the result in /tmp\n");
	printf("\tfnc -t beam -o /tmp foo.ifn: same as before but with the type set\n");
	printf("\tfnc -t erl foo.ifn: translate foo.ifn to erlang\n");
	printf("\tfnc -r foo run: runs the run function in the foo module\n");
}

int is_dir(const char *path) {
	struct stat st;
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int fn_run(const char *args, char *argv0, int is_test) {
	int count, status;
	char *fnpath, buffer[STR_BUFFER_SIZE], *basepath;
	fnpath = get_efene_path_from_env();
	basepath = dirname(dirname(argv0));

	if (fnpath == NULL) {
		if (!is_dir("../ebin")) {

			count = snprintf(buffer, STR_BUFFER_SIZE, "%s/ebin", basepath);
			assert(count <= STR_BUFFER_SIZE);

			fnpath = strdup(buffer);
			assert(fnpath != NULL);

			if (!is_dir(fnpath)) {
				fprintf(stderr, "$FNPATH is not defined, %s and ../ebin don't exist, options:\n", fnpath);
				fprintf(stderr, " * set $FNPATH to the path where efene is installed\n");
				fprintf(stderr, " * run fnc -s from the bin directory\n");
				fprintf(stderr, " * stop doing maginc tricks with your path\n");
				status = EXIT_FAILURE;
			}
			else {
				count = snprintf(buffer, STR_BUFFER_SIZE,
					"erl -run %s -run init stop -noshell -pa \"%s\"",
					args, fnpath);

				assert(count <= STR_BUFFER_SIZE);

				if (is_test) {
					printf("%s\n", buffer);
				}
				else {
					status = system(buffer);
				}
			}
		}
		else {
			count = snprintf(buffer, STR_BUFFER_SIZE,
				"erl -run %s -run init stop -noshell -pa \"../ebin\"",
				args);

			assert(count <= STR_BUFFER_SIZE);

			if (is_test) {
				printf("%s\n", buffer);
			}
			else {
				status = system(buffer);
			}
		}
	}
	else {
		count = snprintf(buffer, STR_BUFFER_SIZE,
			"erl -run %s -run init stop -noshell -pa \"%s/ebin\"",
			args, fnpath);

		assert(count <= STR_BUFFER_SIZE);

		if (is_test) {
			printf("%s\n", buffer);
		}
		else {
			status = system(buffer);
		}

		free(fnpath);
	}

	exit(status);
}

struct FnOptions* fn_options_new() {
	struct FnOptions* options = (struct FnOptions*) malloc(sizeof(struct FnOptions));

	assert(options != NULL);
	options->output_path = NULL;
	options->files = NULL;
	options->files_num = 0;
	options->output_type = NULL;
	options->mode = '?';
	options->is_test = 0;
	options->appends  = (char*) malloc(sizeof(char) * PATHS_SIZE);
	options->prepends = (char*) malloc(sizeof(char) * PATHS_SIZE);
	options->appends[0]  = '\0';
	options->prepends[0] = '\0';

	return options;
}

void fn_options_free_files(struct FnOptions* options) {
	int index;

	if (options != NULL && options->files != NULL && options->files_num > 0) {
		for (index = 0; index < options->files_num; index++) {
			free(options->files[index]);
		}

		free(options->files);
	}
}

void fn_options_delete(struct FnOptions* options) {
	if (options != NULL) {

		if (options->output_path != NULL) {
			free(options->output_path);
		}

		fn_options_free_files(options);

		if (options->output_type != NULL) {
			free(options->output_type);
		}

		free(options->appends);
		free(options->prepends);
		free(options);
	}
}

void fn_options_print(struct FnOptions* options) {
	int index;

	if (options != NULL) {
		printf("options:\n");

		if (options->output_path != NULL) {
			printf("\toutput: %s\n", options->output_path);
		}

		if (options->files != NULL && options->files_num > 0) {
			for (index = 0; index < options->files_num; index++) {
				printf("\tfile: %s\n", options->files[index]);
			}
		}

		if (options->output_type != NULL) {
			printf("\toutput type: %s\n", options->output_type);
		}
	}
	else {
		printf("options: NULL\n");
	}
}

int fn_options_copy_output_type(struct FnOptions* options, const char* arg) {
	if (arg == NULL || options == NULL) {
		return 0;
	}

	if (options->output_type != NULL) {
		free(options->output_type);
	}

	options->output_type = strdup(optarg);
	assert(options->output_type != NULL);

	return 1;
}

int fn_options_copy_output_path(struct FnOptions* options, const char* arg) {
	if (arg == NULL || options == NULL) {
		return 0;
	}

	if (options->output_path != NULL) {
		free(options->output_path);
	}

	options->output_path = strdup(optarg);
	assert(options->output_path != NULL);

	return 1;
}

void fn_options_copy_extra_args(struct FnOptions* options, int optind, int argc, char **argv) {
	int index;

	if (options == NULL) {
		return;
	}

	if (optind == argc) {
		return;
	}

	fn_options_free_files(options);

	options->files_num = argc - optind;
	options->files = (char**) malloc(sizeof(char*) * options->files_num);

	for (index = optind; index < argc; index++) {
		options->files[index - optind] = strdup(argv[index]);
		assert(options->files[index - optind] != NULL);
	}
}

void set_mode(struct FnOptions *options, char new_mode) {
	if (options->mode != '?') {
		fprintf(stderr, "warning! overriding %c flag with %c flag\n", options->mode, new_mode);
	}

	options->mode = new_mode;
}

struct FnOptions* parse_options (int argc, char **argv) {
	int c;
	struct FnOptions* options = fn_options_new();

	while ((c = getopt (argc, argv, "o:ht:sTra:p:")) != -1) {
		switch (c) {
			case 'a':
				if (options->appends[0] == '\0') {
					strcat(options->appends, " -pa");
				}

				strcat(options->appends, " ");
				strncat(options->appends, optarg, PATHS_SIZE);
				break;
			case 'p':
				if (options->prepends[0] == '\0') {
					strcat(options->prepends, " -pz");
				}

				strcat(options->prepends, " ");
				strncat(options->prepends, optarg, PATHS_SIZE);
				break;
			case 'o':
				if (fn_options_copy_output_path(options, optarg) == 0) {
					fprintf(stderr, "error copying output path\n");
				}

				break;
			case 'h':
				show_usage();
				exit(EXIT_SUCCESS);
			case 'T':
				options->is_test = 1;
				break;
			case 'r':
				set_mode(options, 'r');
				break;
			case 't':
				if (fn_options_copy_output_type(options, optarg) == 0) {
					fprintf(stderr, "error copying output type\n");
				}

				break;
			case 's':
				set_mode(options, 's');
				break;
			case '?':
				if (optopt == 'o') {
					fprintf(stderr, "output path option requires an argument\n");
				}
				else if (optopt == 't') {
					fprintf(stderr, "output type option requires an argument\n");
				}
				else if (isprint(optopt)) {
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				}
				else {
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				}

				exit(EXIT_FAILURE);
		}
	}

	if (options->output_type == NULL) {
		options->output_type = strdup("beam");
	}

	if (options->output_path == NULL) {
		options->output_path = strdup(".");
	}

	fn_options_copy_extra_args(options, optind, argc, argv);

	return options;
}

char* str_join(int count, char **strs) {
	int i, size = 0, len, offset = 0;
	char *result;
	assert(count > 0);
	assert(strs != NULL);

	for (i = 0; i < count; i++) {
		// the + 1 is for the spaces in all and for \0 in the last one
		size += strlen(strs[i]) + 1;
	}

	result = (char*) malloc(sizeof(char) * size);
	assert(result != NULL);

	for (i = 0; i < count; i++) {
		len = strlen(strs[i]);
		strncpy(result + offset, strs[i], len);
		offset += len;
		result[offset] = ' ';
		offset++;
	}

	result[offset - 1] = '\0';
	return result;
}

int main (int argc, char **argv) {
	int count, is_test;
	char buffer[STR_BUFFER_SIZE], *extra_args;
	struct FnOptions* options = parse_options(argc, argv);

	if (options->mode == 's') {
		count = snprintf(buffer, STR_BUFFER_SIZE, "fn run shell%s%s",
				options->appends, options->prepends);
	}
	else {
		if (options->files_num == 0) {
			fn_options_delete(options);
			fprintf(stderr, "at least one extra argument required\n");
			return EXIT_FAILURE;
		}

		extra_args = str_join(options->files_num, options->files);

		if (options->mode == 'r') {
			if (options->files_num < 2) {
				fprintf(stderr, "at least two arguments required, got %d\n",
						options->files_num);
				fprintf(stderr, " -r module function [arguments*]\n");
				fprintf(stderr, " -r module function [arguments*]\n");
				return EXIT_FAILURE;
			}

			count = snprintf(buffer, STR_BUFFER_SIZE, "%s%s%s", extra_args,
					options->appends, options->prepends);
		}
		else if (strcmp(options->output_type, "beam") == 0) {
			count = snprintf(buffer, STR_BUFFER_SIZE, "fn run %s \"%s\" %s",
					options->output_type, options->output_path, extra_args);
		}
		else {
			count = snprintf(buffer, STR_BUFFER_SIZE, "fn run %s %s",
					options->output_type, extra_args);
		}

		free(extra_args);
	}

	assert(count <= STR_BUFFER_SIZE);
	is_test = options->is_test;
	fn_options_delete(options);

	return fn_run(buffer, argv[0], is_test);
}
