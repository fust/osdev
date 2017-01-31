#include "console/console.h"
#include "stdio.h"
#include "dev/kbd.h"
#include "sys/pipe.h"
#include "mem/kmalloc.h"
#include "stdio.h"
#include "string.h"
#include "ds/hashtable.h"
#include "stdbool.h"
#include "fs/vfs.h"
#include "debug.h"

typedef void (*console_func_t)(int argc, char *argv[]);

uint8_t *kbbuffer;

hashtable_t *command_map;

uint8_t map_keycode_to_char(uint8_t keycode);
void process_command(const char *command);

// Internal commands
void console_help(int argc, char *argv[]);
void console_echo(int argc, char *argv[]);
void console_mount(int argc, char *argv[]);

uint8_t map_us[128] = {
		0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
		'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
		'\n',
		0,
		'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
		'`',
		0,
		'\\',
		'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
		'*',
		0, ' ',
		0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0,
		0,
		'7', '8', '9',
		'-',
		'4', '5', '6', '+',
		'1', '2', '3',
		'0', '.',
		0,
		0,
		0,
		0, 0,
};

void console_run()
{
	command_map = hashtable_create(3);

	hashtable_insert(command_map, "help", 0, console_help);
	hashtable_insert(command_map, "echo", 0, console_echo);
	hashtable_insert(command_map, "mount", 0, console_mount);

	uint16_t current = 0;
	kbbuffer = (uint8_t *)kmalloc(sizeof(uint8_t) * 256); // 256 byte keyboard buffer

	kprintf("\n================= Kernel console =================\n\n> ");

	kbd_event_t *buff = (kbd_event_t *)kmalloc(sizeof(kbd_event_t));
	vfs_node_t *kbdnode = kopen("/dev/kbd");
	if (!kbdnode) {
		kprintf("Could not open /dev/kbd. Exiting.\n");
	}

	pipe_t *kbd_pipe = (pipe_t *)kbdnode->device;
	debug("Keyboard pipe is at 0x%x\n", kbd_pipe);

	while (1) {
		while (vfs_read(kbdnode, 0, sizeof(kbd_event_t), (uint8_t *)buff) != 0) {
			__asm__ __volatile__("hlt");
		}

		uint8_t c = map_keycode_to_char(buff->keycode);
		if (c == 0) {
			continue;
		}

		kprintf("%c", c);

		if (c == '\b' && current > 0) {
			kbbuffer[current] = ' ';
			current--;
		} else if (c != '\n') {
			kbbuffer[current] = c;
			current++;
		} else {
			kbbuffer[current] = '\0';
			process_command((const char *)kbbuffer);
			memset(kbbuffer, 0, sizeof(uint8_t) * 256);
			current = 0;
			kprintf("> ");
		}
	}
}

uint8_t map_keycode_to_char(uint8_t keycode)
{
	return map_us[keycode];
}

/* Released to public domain */
/* Simple argument parsing - returns an array of char* pointers
@str is the string to be parsed
@delim is a string containing characters to split by
@escape is a string containing characters to escape ("\"\'\n", etc) - optional
@_argc is a pointer to integer containing the size of returned pointer array */

char **tokenize(const char *str, const char *delim, const char *escape, int *_argc) {
   if (!str || !_argc || !delim) {
      return NULL;
   }

   int i, argc = 0;
   char *split = strdup(str);

   /* first pass, approximate how many arguments there are for allocation */
   while(*str) {
      if (strspn(str, delim)) {
         argc++;
         str += strspn(str, delim);
      }
      str++;
   }

   /* allocate a pointer array of the proper size */
   char **ret = (char **)kmalloc(sizeof(char *) * argc);
   argc = 0;
   ret[argc++] = split;

   /* second pass, split strings by the delimiter */
   while(*split) {
      if (escape && strspn(split, escape)) {
         i = strspn(split, escape);
         /* save the escaped character for matching */
         char c = *split;

         split += i;
         /* if we can't find a matching character to escape, ignore it */
         if (!strchr(split, c)) {
            ret[argc++] = split - i;
            split++;
            continue;
         }
         *(split - i) = '\0';
         ret[argc++] = split;
         /* pointer to first matching character */
         split = strchr(split, c);
         *split = '\0';
         split++;
         /* Make sure we don't return the last character as an argument */
         if (!strspn(split, delim) && (strlen(split) > 1)) {
            ret[argc++] = split;
         }
      }

      if (strspn(split, delim)) {
         i = strspn(split, delim);
         *split = '\0';
         split += i;
         /* if there's an escaped character, try to escape it */
         if (escape && strspn(split, escape)) {
            continue;
         }
         ret[argc++] = split;
      } else {
         split++;
      }
   }
   *_argc = argc;
   return ret;
}

char *parse_command(const char *command)
{
	char *buf = (char *)kmalloc(64);

	int i = 0;
	while (command[i] != ' ' && command[i] != '\0') {
		buf[i] = command[i];
		i++;
	}

	return buf;
}

void process_command(const char *command)
{
	char *cmd = parse_command(command);
	int argc = (int) kmalloc(sizeof(int));
	char** argv = tokenize(command, " ", "\"", &argc);

	hashtable_entry_t *entry = hashtable_lookup(command_map, cmd);
	if (entry) {
		console_func_t func = (console_func_t)entry->value;
		func(argc, argv);
	} else {
		kprintf("Command not found: %s\n", cmd);
	}

	kfree(&argc);
	kfree(argv);
}

void console_help(int argc, char *argv[])
{
	kprintf("\
OS Help.\n\
Available commands:\n\n\
echo\t\tEcho back the contents of the first argument.\n\
mount\t\tDisplay the mounted filesystems\
help\t\tDisplay this info screen.\n");

}

void console_echo(int argc, char *argv[])
{
	for (int i = 1; i < argc; ++i) {
		kprintf("%s ", argv[i]);
	}
	kprintf("\n");
}

void mount_walker (hashtable_t *table, hashtable_entry_t *entry)
{
	kprintf("%s is mounted at %s\n", entry->key, entry->value);
}

void console_mount(int argc, char *argv[])
{
	if (argc == 1) { // Print the mounted filesystems
		hashtable_t *mounts = vfs_get_mount_table();
		hashtable_walk(mounts, &mount_walker);
	}
}
