
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define debug(f,a...)	do{if(debug_level>0)printf (f,##a);}while(0)
static int debug_level;

enum log_level
{
	log_level_error	= 0,
	log_level_warning,
	log_level_noti,
	log_level_info,
	log_level_debug,
	log_level_trace,

	log_level_user1,
	log_level_user2,
	log_level_user3,

	log_level_max = log_level_user3,
};

enum log_type
{
	log_type_aline,
	log_type_binary,
};

struct header
{
	unsigned long long clock;
	unsigned int module; //struct log_module *module;
	enum log_level level;

	/* data type and size that fallowing this header */
	enum log_type type;
	int size;

	/* for binary */
	unsigned int bin_data; //const void *bin_data;
	int bin_size;

	/* continuty counter to check buffer drop */
	int count;

	char paddings[8];
};
#define sizeof_header	40

static const char *level_name (enum log_level l)
{
	switch (l)
	{
	default:
	case log_level_error:
		return "ERROR";
	case log_level_warning:
		return "WARN";
	case log_level_noti:
		return "NOTI";
	case log_level_info:
		return "INFO";
	case log_level_debug:
		return "DEBUG";
	case log_level_trace:
		return "TRACE";

	case log_level_user1:
		return "USER1";
	case log_level_user2:
		return "USER2";
	case log_level_user3:
		return "USER3";
	}
}

static void dump (const char *name, void *_data, int len)
{
	int a, b;
	unsigned char *data = _data;

	if( name != NULL && name[0] != 0 )
		puts( name );

	for( a=0, b=0; a<len; )
	{
		if( a%16 == 0 )
			printf( "%04x:", a );
		printf( " %02x", data[a] );
		a++;
		if( a%16 == 0 )
		{
			fputs( "  ", stdout );
			for( ; b<a; b++ )
				putchar( (' ' <= data[b] && data[b] <= '~')?data[b]:'.' );
			fputs( "\n", stdout );
		}
	}

	if( a%16 != 0 )
	{
		for( ; a%16 != 0; a++ )
			fputs( "   ", stdout );

		fputs( "  ", stdout );
		for( ; b<len; b++ )
			putchar( (' ' <= data[b] && data[b] <= '~')?data[b]:'.' );
		fputs( "\n", stdout );
	}

	return;
}

static int search_header (int in)
{
	char buf[sizeof(struct header)];
	struct header *header;
	int readed;

	header = (struct header*)buf;
	readed = read (in, buf, sizeof_header);
	if (readed != sizeof_header)
	{
		fprintf (stderr, "no more header\n");
		return -1;
	}

	do
	{
		if (header->level > log_level_max)
			goto not_header;
		if (header->size > 1024*1024)
			goto not_header;
		if (header->type > log_type_binary)
			goto not_header;
		if (header->bin_data != 0 && header->bin_size == 0)
			goto not_header;
		if (header->bin_data == 0 && header->bin_size != 0)
			goto not_header;

		lseek (in, -sizeof_header, SEEK_CUR);
		break;

not_header:
		memmove (buf, buf+8, sizeof(buf)-8);
		readed = read (in, buf+sizeof_header-8, 8);
		if (readed != 8)
		{
			fprintf (stderr, "no more header..\n");
			return -1;
		}
	}
	while (1);

	return 0;
}

int main (int argc, char **argv)
{
	char *filename;
	int in;
	int lines;
	int opt;

	filename = "/dev/stdin";
	debug_level = 0;
	while ((opt = getopt (argc, argv, "-d")) != -1)
	{
		switch (opt)
		{
		default:
		case '?':
			printf ("logfunnel internal buffer parser.\n"
				" # ./buffer2text <arguments> ... <buffer_dump.bin>\n"
				"arguments:\n"
				"  -d     : enable debuging message\n"
				"  <buffer_dump.bin> : filename of binary dump of \"buffer\" pointer\n"
			       );
			return 1;

		case 'd':
			debug_level ++;
			break;

		case 1:
			filename = optarg;
			break;
		}
	}

	in = open (filename, O_RDONLY);
	if (in < 0)
	{
		fprintf (stderr, "cannot open input file, %s, %s\n",
				filename, strerror(errno));
		return 1;
	}

	lines = 0;
	while (1)
	{
		struct header header;
		unsigned long long t;
		unsigned long nanosec_rem;
		int readed;
		char *buf;
		int len;
		char padding[8];

		readed = read (in, &header, sizeof_header);
		if (readed != sizeof_header)
			break;

		debug ("size %d\n", header.size);
		debug ("level %d\n", header.level);
		debug ("type %d\n", header.type);
		debug ("count %d\n", header.count);
		if (header.size < 0)
		{
			debug ("end marker\n");
			break;
		}

		if (header.size >= 1024*1024)
		{
			fprintf (stderr, "too big line size?? %d\n", header.size);
			lseek (in, -sizeof_header, SEEK_CUR);
			if (debug_level>0)
				dump ("", &header, sizeof(header));
			if (search_header (in) < 0)
				return 0;
			continue;
		}

		buf = malloc (header.size+1);
		if (buf == NULL)
		{
			fprintf (stderr, "malloc failed, %d, %s\n",
					header.size, strerror(errno));
			break;
		}

		readed = read (in, buf, header.size);
		if (readed < header.size)
		{
			fprintf (stderr, "line read failed, %d, %s\n",
					header.size, strerror(errno));
			lseek (in, -readed, SEEK_CUR);
			if (debug_level > 0)
				dump ("", &header, sizeof(header));
			if (search_header (in) < 0)
				return 0;
			continue;
		}
		buf[header.size] = 0;
		debug ("got %d characters\n", readed);
		if (debug_level>0)
			dump ("", buf, header.size>32?32:header.size);

		t = header.clock;
		nanosec_rem = t % 1000000000;
		t = t / 1000000000;
		printf ( "[%5lu.%06lu] %10x %-5s %s\n",
				(unsigned long)t, nanosec_rem/1000,
				header.module,
				level_name (header.level),
				buf);

		free (buf);
		lines ++;

		len = ((sizeof_header+header.size+7)&~7)
			- (sizeof_header+header.size);
		debug ("%d padding\n", len);
		read (in, padding, len);
	}

	printf ("total %d lines\n", lines);

	return 0;
}

