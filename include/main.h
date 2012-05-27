#ifndef MAIN_H
#define MAIN_H

#include <command.h>

struct main_state {
	void *imageaddr;
	int want_repeat;
};

int main_process_command(struct main_state *res);

int	run_command(const char *cmd, int repeat, int *want_repeat);

int	readline(const char *const prompt);
int	readline_into_buffer(const char *const prompt, char * buffer);
int	parse_line (char *, char *[]);
void init_cmd_timeout(void);
void reset_cmd_timeout(void);

#endif
