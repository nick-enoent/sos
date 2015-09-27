/*
 * Copyright (c) 2015 Open Grid Computing, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the BSD-type
 * license below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *      Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *      Neither the name of Open Grid Computing nor the names of any
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *      Modified source versions must be plainly marked as such, and
 *      must not be misrepresented as being the original software.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *
 *
 * \section sos_part_query sos_part_query command
 *
 * \b NAME
 *
 * sos_part_query - Query a container's partitions
 *
 * \b SYNOPSIS
 *
 * sos_part_query <container>
 *
 * \b DESCRIPTION
 *
 * List the partitions defined in a Container. For example:
 *
 *      tom@css:/SOS/import$ sos_part /NVME/0/SOS_ROOT/Test
 *      Partition Name       RefCount Status           Size     Modified         Accessed         Path
 *      -------------------- -------- ---------------- -------- ---------------- ---------------- ----------------
 *      00000000                    3 ONLINE                 1M 2015/08/25 13:49 2015/08/25 13:51 /SOS_STAGING/Test
 *      00000001                    3 ONLINE                 2M 2015/08/25 11:54 2015/08/25 13:51 /NVME/0/SOS_ROOT/Test
 *      00000002                    3 ONLINE                 2M 2015/08/25 11:39 2015/08/25 13:51 /NVME/0/SOS_ROOT/Test
 *      00000003                    3 ONLINE PRIMARY         2M 2015/08/25 11:39 2015/08/25 13:51 /NVME/0/SOS_ROOT/Test
 *
 * \b PATH
 *
 * Specify the PATH to the Container. This option is required.
 *
 */
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sos/sos.h>

void usage(int argc, char *argv[])
{
	printf("sos_part_query <path>\n");
	printf("    <path>   The path to the container.\n");
	exit(1);
}

const char *pretty_file_size(off_t size)
{
	int i;
	static char buf[32];
	const char sz_strs[] = { ' ', 'K', 'M', 'G', 'T' };
	size_t str_cnt = sizeof(sz_strs) / sizeof(sz_strs[0]);

	for (i = 0; i < str_cnt; i++) {
		if (size < 1000 || str_cnt == 4) {
			sprintf(buf, "%ld%c", size, sz_strs[i]);
			return buf;
		}
		size = size / 1000;
	}
	return NULL;		/* NB: can't happen */
}

void part_list(sos_t sos, FILE *fp)
{
	struct sos_part_stat_s sb;
	sos_part_t part;
	sos_part_iter_t iter;

	if (!fp)
		fp = stdout;

	iter = sos_part_iter_new(sos);
	if (!iter)
		return;

	fprintf(fp, "%-20s %-8s %-16s %-8s %-16s %-16s %s\n", "Partition Name", "RefCount",
		"Status", "Size", "Modified", "Accessed", "Path");
	fprintf(fp, "-------------------- -------- ---------------- "
		"-------- ---------------- ---------------- ----------------\n");
	for (part = sos_part_first(iter); part; part = sos_part_next(iter)) {

		char *statestr;
		fprintf(fp, "%-20s %8d ", sos_part_name(part), sos_part_refcount(part));
		switch (sos_part_state(part)) {
		case SOS_PART_STATE_OFFLINE:
			statestr = "OFFLINE";
			break;
		case SOS_PART_STATE_ACTIVE:
			statestr = "ACTIVE";
			break;
		case SOS_PART_STATE_PRIMARY:
			statestr = "PRIMARY";
			break;
		case SOS_PART_STATE_MOVING:
			statestr = "MOVING";
			break;
		default:
			statestr = "!UNKNOWN!";
		}
		fprintf(fp, "%-16s ", statestr);

		int rc = sos_part_stat(part, &sb);
		char datestr[80];
		struct tm *tm;

		if (!rc) {
			fprintf(fp, "%8s ", pretty_file_size(sb.size));

			tm = localtime(&sb.modified);
			strftime(datestr, sizeof(datestr), "%Y/%m/%d %H:%M", tm);
			fprintf(fp, "%-16s ", datestr);

			tm = localtime(&sb.accessed);
			strftime(datestr, sizeof(datestr), "%Y/%m/%d %H:%M", tm);
			fprintf(fp, "%-16s ", datestr);
		} else
			fprintf(fp, "%42s ", "");
		fprintf(fp, "%s\n", sos_part_path(part));
		sos_part_put(part);
	}
	sos_part_iter_free(iter);
}

int main(int argc, char **argv)
{
	sos_t sos;

	if (argc < 2)
		usage(argc, argv);

	sos = sos_container_open(argv[1], SOS_PERM_RO);
	if (!sos) {
		printf("Error %d opening the container %s.\n",
		       errno, argv[1]);
		exit(1);
	}
	part_list(sos, stdout);
	return 0;
}