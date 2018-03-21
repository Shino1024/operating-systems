#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>

int error_code;

typedef enum comparator {
	EARLIER = '<',
	EQUAL = '=',
	LATER = '>',
	UNDEFINED_COMP = '\0'
} comparator;

typedef char * filepath;

typedef struct search_criteria {
	filepath path;
	comparator comp;
	time_t mod_date;
} search_criteria;

int free_criteria(search_criteria *criteria) {
	if (criteria == NULL) {
		return -1;
	}

	if (criteria->path == NULL) {
		free(criteria);
		return -2;
	}

	free(criteria->path);
	free(criteria);

	return 0;
}

char * stringify_permissions(unsigned int permissions) {
	char *str_ret = (char *) calloc(10, sizeof(char));
	if (str_ret == NULL) {
		return NULL;
	}

	unsigned int permission_masks[] = {
		S_IRUSR,
		S_IWUSR,
		S_IXUSR,
		S_IRGRP,
		S_IWGRP,
		S_IXGRP,
		S_IROTH,
		S_IWOTH,
		S_IXOTH
	};
	char permission_letters[] = {
		'r',
		'w',
		'x'
	};
	unsigned int i;
	for (i = 0; i < sizeof(permission_masks) / sizeof(*permission_masks); ++i) {
		str_ret[i] = permissions & permission_masks[i] ? permission_letters[i % (sizeof(permission_letters) / sizeof(*permission_letters))] : '-';
	}

	return str_ret;
}

int print_file_info(const char *absolute_filepath, struct stat file_info) {
	time_t modify_time_copy = file_info.st_mtime;
	struct tm *mod_tm = localtime(&modify_time_copy);
	char *string_permissions = stringify_permissions(file_info.st_mode);
	if (string_permissions == NULL) {
		return -1;
	}

	printf(" %-80s | %10ld B | %s | %02d/%02d/%04d\n",
			absolute_filepath,
			file_info.st_size,
			string_permissions,
			mod_tm->tm_mday, mod_tm->tm_mon + 1, mod_tm->tm_year + 1900);
	
	free(string_permissions);

	return 0;
}

int meets_criteria(const search_criteria *criteria, time_t modify_time) {
	if (criteria == NULL) {
		return -1;
	}

	time_t modify_time_copy = modify_time;
	struct tm modify_time_struct = { 0 };
	localtime_r(&modify_time_copy, &modify_time_struct);

	time_t mod_date_copy = criteria->mod_date;
	struct tm given_time_struct = { 0 };
	localtime_r(&mod_date_copy, &given_time_struct);

	switch (criteria->comp) {
		case EARLIER:
			if (given_time_struct.tm_year > modify_time_struct.tm_year
					|| (given_time_struct.tm_year == modify_time_struct.tm_year
						&& given_time_struct.tm_mon > modify_time_struct.tm_mon)
					|| (given_time_struct.tm_year == modify_time_struct.tm_year
						&& given_time_struct.tm_mon == modify_time_struct.tm_mon
						&& given_time_struct.tm_mday > modify_time_struct.tm_mday)) {
				return 0;
			} else {
				return 1;
			}
			break;

		case EQUAL:
			if (given_time_struct.tm_year == modify_time_struct.tm_year
					&& given_time_struct.tm_mon == modify_time_struct.tm_mon
					&& given_time_struct.tm_mday == modify_time_struct.tm_mday) {
				return 0;
			} else {
				return 1;
			}
			break;

		case LATER:
			if (given_time_struct.tm_year < modify_time_struct.tm_year
					|| (given_time_struct.tm_year == modify_time_struct.tm_year
						&& given_time_struct.tm_mon < modify_time_struct.tm_mon)
					|| (given_time_struct.tm_year == modify_time_struct.tm_year
						&& given_time_struct.tm_mon == modify_time_struct.tm_mon
						&& given_time_struct.tm_mday < modify_time_struct.tm_mday)) {
				return 0;
			} else {
				return 1;
			}
			break;

		default:
			return 2;
	}

	return 2;
}

int recursive_ls(const search_criteria *criteria, const filepath absolute_given) {
	struct dirent *entry;
	DIR *dir;

	char absolute_path[PATH_MAX + 3] = { 0 };
	if (absolute_given == NULL) {
		if (realpath(criteria->path, absolute_path) == NULL) {
			return -1;
		}
		unsigned int eraser;
		for (eraser = strlen(absolute_path); eraser < PATH_MAX + 3; ++eraser) {
			absolute_path[eraser] = '\0';
		}
	} else {
		strcpy(absolute_path, absolute_given);
	}

	unsigned int relative_begin_index = strlen(absolute_path);
	if (relative_begin_index >= PATH_MAX - 1) {
		return -2;
	}

	if (absolute_path[relative_begin_index] != '/') {
		absolute_path[relative_begin_index] = '/';
		++relative_begin_index;
	}

	dir = opendir(absolute_path);
	if (dir == NULL) {
		printf("Couldn't enter %s, omitting...\n\n", absolute_path);
		return -3;
	}

	printf("\nList of %s:\n", absolute_path);
	printf(" %-80s | %12s | %9s | %10s\n",
			"ABSOLUTE NAME",
			"SIZE",
			"PERM",
			"MODIFIED");
	char current_filepath_buffer[PATH_MAX + 3] = { 0 };
	strcpy(current_filepath_buffer, absolute_path);
	struct stat file_info;
	do {
		entry = readdir(dir);
		if (entry != NULL) {
			strcat(current_filepath_buffer + relative_begin_index, entry->d_name);
			error_code = lstat(current_filepath_buffer, &file_info);
			if (error_code < 0) {
				printf("Couldn't read file %s, omitting...\n", current_filepath_buffer);
			} else {
				if (S_ISREG(file_info.st_mode)
						&& meets_criteria(criteria, file_info.st_mtime) == 0) {
					print_file_info(current_filepath_buffer, file_info);
				}
			}

			unsigned int eraser = relative_begin_index;
			while (current_filepath_buffer[eraser] != '\0') {
				current_filepath_buffer[eraser] = '\0';
				++eraser;
			}
		}
	} while (entry != NULL);

	rewinddir(dir);
	do {
		entry = readdir(dir);
		if (entry != NULL) {
			if (strcmp(entry->d_name, ".") == 0
					|| strcmp(entry->d_name, "..") == 0) {
				continue;
			}

			strcat(current_filepath_buffer + relative_begin_index, entry->d_name);
			error_code = lstat(current_filepath_buffer, &file_info);
			if (error_code < 0) {
				printf("Couldn't read file %s, omitting...\n", current_filepath_buffer);
			} else {
				if (S_ISDIR(file_info.st_mode)) {
					recursive_ls(criteria, current_filepath_buffer);
				}
			}

			unsigned int eraser = relative_begin_index;
			while (current_filepath_buffer[eraser] != '\0') {
				current_filepath_buffer[eraser] = '\0';
				++eraser;
			}
		}
	} while (entry != NULL);

	closedir(dir);
	return 0;
}

int check_filepath(const filepath folder) {
	struct stat test;
	error_code = lstat(folder, &test);
	if (error_code < 0) {
		return -1;
	}
	
	if (!(S_ISDIR(test.st_mode))) {
		printf("The provided path does not point to a directory.\n");
		return -2;
	}

	return 0;
}

int check_comparator(const char *comp) {
	if (strlen(comp) > 1) {
		return -1;
	}

	if (*comp == EARLIER
			|| *comp == EQUAL
			|| *comp == LATER) {
		return 0;
	}

	return -2;
}

int check_date(const char *date) {
	if (date == NULL) {
		return -1;
	}

	unsigned int day, month, year;
	error_code = sscanf(date, "%02d/%02d/%04d", &day, &month, &year);
	if (error_code == 0) {
		printf("The provided time is not in correct format (DD/MM/YYYY).\n");
		return -1;
	}
	
	if (day < 1 || day > 31
			|| month < 1 || month > 12
			|| year < 1900 || year > 2099) {
		printf("Make sure the date is in correct boundaries.\n");
	return -2;
	}

	return 0;
}

int usage(const char *program_name) {
	printf("Usage: %s"
			"\n\tfolder_path (relative or absolute)"
			"\n\tcomparator ('<'|'='|'>')"
			"\n\tdate (format: DD/MM/YYYY)"
			"\n\n", program_name);

	return 0;
}

time_t parse_date(const char *date) {
	unsigned int day, month, year;
	sscanf(date, "%02d/%02d/%04d", &day, &month, &year);

	struct tm tm_date = { 0 };
	tm_date.tm_mday = day;
	tm_date.tm_mon = month - 1;
	tm_date.tm_year = year - 1900;

	time_t ret = mktime(&tm_date);

	return ret;
}

search_criteria * parse_args(int argc, char *argv[]) {
	search_criteria *ret = (search_criteria *) calloc(1, sizeof(*ret));
	if (ret == NULL) {
		return NULL;
	}

	if (argc != 4) {
		usage(argv[0]);
		return NULL;
	}

	error_code = check_filepath(argv[1]);
	if (error_code < 0) {
		printf("Provide a correct folder path.\n");
		return NULL;
	}

	error_code = check_comparator(argv[2]);
	if (error_code < 0) {
		printf("Provide a correct comaprison operator ('<', '=' or '>').\n");
		return NULL;
	}

	error_code = check_date(argv[3]);
	if (error_code < 0) {
		printf("Provide a correct date (format: DD/MM/YYYY)\n");
		return NULL;
	}

	unsigned int filepath_len = strlen(argv[1]);
	ret->path = (filepath) calloc(filepath_len, sizeof(*(ret->path)));
	if (ret->path == NULL) {
		return NULL;
	}
	strcpy(ret->path, argv[1]);
	ret->comp = (comparator) argv[2][0];
	ret->mod_date = parse_date(argv[3]);

	return ret;
}

int main(int argc, char *argv[]) {
	search_criteria *criteria = parse_args(argc, argv);
	if (criteria == NULL) {
		return 1;
	}

	error_code = recursive_ls(criteria, NULL);
	if (error_code < 0) {
		printf("Recursive ls failed. Exiting...\n");
		return 2;
	}
	printf("\n");

	free_criteria(criteria);

	return 0;
}
