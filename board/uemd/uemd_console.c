
#include <common.h>
#include <serial.h>

/* test if ctrl-c was pressed */
static int ctrlc_disabled = 0;
static int ctrlc_was_pressed = 0;

int uemd_console_init(void)
{
	ctrlc_disabled = 0;
	ctrlc_was_pressed = 0;

	return serial_init();
}

int getc(void)
{
	return serial_getc();
}

int tstc(void)
{
	return serial_tstc();
}

void putc(const char c)
{
	serial_putc(c);
}

void nputc(const char *c, size_t n)
{
	int i;
	for(i=0;i<n;i++)
		putc(c[i]);
}

void puts(const char *s)
{
	serial_puts(s);
}

int printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

	va_start(args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	/* Print the string */
	puts(printbuffer);
	return i;
}

int vprintf(const char *fmt, va_list args)
{
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);

	/* Print the string */
	puts(printbuffer);
	return i;
}

int ctrlc(void)
{
	if(ctrlc_was_pressed)
		return 1;

	if (!ctrlc_disabled) {
		if (tstc()) {
			switch (getc()) {
			case 0x03:		/* ^C - Control C */
				ctrlc_was_pressed = 1;
				return 1;
			default:
				break;
			}
		}
	}

	return 0;
}

int disable_ctrlc(int disable)
{
	int prev = ctrlc_disabled;	/* save previous state */

	ctrlc_disabled = disable;
	return prev;
}

int had_ctrlc (void)
{
	return ctrlc_was_pressed;
}

void clear_ctrlc(void)
{
	ctrlc_was_pressed = 0;
}

