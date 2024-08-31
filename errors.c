#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "errors.h"
#include "asc.h"
#include "init.h"
#include "io/elpathwrapper.h"

int clear_log = 0;

FILE* open_log (const char *fname, const char *mode)
{
	FILE *file = open_file_config (fname, mode);
	char starttime[200], sttime[200];
	struct tm *l_time; time_t c_time;
	if (file == NULL)
	{
		fprintf (stderr, "Unable to open log file \"%s\"\n", fname);
		exit (1);
	}

	time (&c_time);
	l_time = localtime (&c_time);
	strftime(sttime, sizeof(sttime), "\n\nLog started at %Y-%m-%d %H:%M:%S localtime", l_time);
	safe_snprintf(starttime, sizeof(starttime), "%s (%s)\n\n", sttime, tzname[l_time->tm_isdst>0]);
	fwrite (starttime, strlen(starttime), 1, file);
	return file;
}



FILE *conn_file = NULL;
void clear_conn_log()
{
	if(!conn_file) {
		conn_file = open_log ("connection_log.txt", "w");
	}
	fflush (conn_file);
}

void log_conn(const Uint8 *in_data, Uint16 data_length)
{
  	if(!conn_file) {
		conn_file = open_log ("connection_log.txt", "a");
	}
  	fwrite (in_data, data_length, 1, conn_file);
  	fflush (conn_file);
}

