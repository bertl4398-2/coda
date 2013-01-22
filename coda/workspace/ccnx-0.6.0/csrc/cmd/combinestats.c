/** 
 * Copyright (c) Francisco de Meneses Neves Ramos dos Santos
 * Email: francisco.santos@epfl.ch
 * Date: 7/9/2012
 */

/* print files in current directory in reverse order */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ccn/hashtb.h>

#define MAXLINELEN 512

struct stats_entry {
	const char *name;
	unsigned long num_req;
	unsigned long sat_req;
};

int selector(const struct dirent* dir) {
	if (dir->d_type == DT_DIR) {
		return (strncmp(dir->d_name, "files-", 6) == 0);
	}
	return 0;
}

void read_stats(struct hashtb* stats, const char * filename) {
	char buf[MAXLINELEN];
	char name[MAXLINELEN];
	unsigned long num_req = 0;
	unsigned long sat_req = 0;
	struct hashtb_enumerator ee;
	struct hashtb_enumerator * hte = &ee;
	struct stats_entry* entry = NULL;
	int res = 0;

	memset(hte, 0, sizeof(struct hashtb_enumerator));

	FILE* input = fopen(filename, "r");
	if (input == NULL) {
		return;
	}

	while (fgets(buf, MAXLINELEN, input) != NULL) {
		memset(name, 0, MAXLINELEN);
		sscanf(buf, "%s %lu %lu", name, &num_req, &sat_req);
		hashtb_start(stats, hte);
		res = hashtb_seek(hte, name, MAXLINELEN, 0);
		entry = hte->data;
		if (res == HT_NEW_ENTRY) {
			entry->name = hte->key;
			entry->num_req = 0;
			entry->sat_req = 0;
		}
		entry->num_req += num_req;
		entry->sat_req += sat_req;
		hashtb_end(hte);
	}

	fclose(input);
}

void output_statistics(struct hashtb* stats, const char * filename) {
	struct hashtb_enumerator ee;
	struct hashtb_enumerator* hte = &ee;
	struct stats_entry * entry = NULL;
	FILE* output = NULL;

	output = fopen(filename, "w");
	if (output == NULL) {
		fprintf(stderr, "Could not output stats to file %s.\n", filename);
		return;
	}

	fprintf(output, "Content \"Total Requests\" \"Satisfied Requests\"\n");

	hashtb_start(stats, hte);
	for (; hte->data != NULL; hashtb_next(hte)) {
		entry = (struct stats_entry *) hte->data;
		if (entry->name == NULL) {
			continue;
		}
		if (strlen(entry->name) >= 5
				&& (strncmp(entry->name, "/ccnx", 5) == 0)) {
			continue;
		}
		if (strlen(entry->name) >= 1 && entry->name[0] != '/') {
			continue;
		}
		if (strstr(entry->name, "KEY") != NULL) {
			continue;
		}
		fprintf(output, "%s %lu %lu\n", entry->name, entry->num_req, entry->sat_req);
	}
	hashtb_end(hte);
	fclose(output);
}

int main(int argc, char* argv[]) {
	struct dirent **namelist;
	int n;
	struct hashtb* stats = NULL;
	char filename[512];

	stats = hashtb_create(sizeof(struct stats_entry), NULL);

	n = scandir(".", &namelist, &selector, alphasort);
	if (n < 0)
		perror("scandir");
	else {
		while (n--) {
			sprintf(filename, "%s/stats.csv", namelist[n]->d_name);
			read_stats(stats, filename);
			free(namelist[n]);
		}
		free(namelist);
	}

	output_statistics(stats, "combined.csv");

	hashtb_destroy(&stats);
	return 0;
}
