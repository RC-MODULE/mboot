#ifndef MAIN_H
#define MAIN_H

#include <command.h>
#include <image.h>

struct main_state {
	struct image_info os_image;
	int want_repeat;
};

static inline void main_state_init(struct main_state *ms)
{
	memset(ms, 0, sizeof(struct main_state));
}

#define MAIN_STATE_HAS_ENTRY(ps) ((ps)->os_image.entry != 0L)

int main_process_command(struct main_state *s);

int run_command(struct main_state *s, const char *cmd, int repeat, int *want_repeat);

int readline(const char *const prompt);
int readline_into_buffer(const char *const prompt, char * buffer);
int parse_line (char *, char *[]);
void init_cmd_timeout(void);
void reset_cmd_timeout(void);

#endif
