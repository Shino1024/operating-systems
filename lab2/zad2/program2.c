#define _XOPEN_SOURCE 500
#include <ftw.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <dirent.h>

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

static search_criteria *global_criteria;

int free_criteria() {
	if (global_criteria == NULL) {
		return -1;
	}

	if (global_criteria->path == NULL) {
		free(global_criteria);
		return -2;
	}

	free(global_criteria->path);
	free(global_criteria);

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

int stat_function(const char *path, const struct stat *file_info, int extra_info, struct FTW *ftw_info) {
	if (S_ISREG(file_info->st_mode)
			&& meets_criteria(global_criteria, file_info->st_mtime) == 0) {
		print_file_info(path, *file_info);
	}
	return 0;
}

int nftw_ls() {
	char absolute_path[PATH_MAX + 3] = { 0 };
	if (realpath(global_criteria->path, absolute_path) == NULL) {
		return -1;
	}

	unsigned int eraser;
	for (eraser = strlen(absolute_path); eraser < PATH_MAX + 3; ++eraser) {
		absolute_path[eraser] = '\0';
	}

	unsigned int relative_begin_index = strlen(absolute_path);
	if (relative_begin_index >= PATH_MAX - 1) {
		return -2;
	}

	if (absolute_path[relative_begin_index] != '/') {
		absolute_path[relative_begin_index] = '/';
		++relative_begin_index;
	}

	printf("\n %-80s | %12s | %9s | %10s\n",
			"ABSOLUTE NAME",
			"SIZE",
			"PERM",
			"MODIFIED");

	nftw(absolute_path, stat_function, 1000, FTW_DEPTH | FTW_PHYS);

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
		free(ret);
		return NULL;
	}

	error_code = check_filepath(argv[1]);
	if (error_code < 0) {
		printf("Provide a correct folder path.\n");
		free(ret);
		return NULL;
	}

	error_code = check_comparator(argv[2]);
	if (error_code < 0) {
		printf("Provide a correct comaprison operator ('<', '=' or '>').\n");
		free(ret);
		return NULL;
	}

	error_code = check_date(argv[3]);
	if (error_code < 0) {
		printf("Provide a correct date (format: DD/MM/YYYY)\n");
		free(ret);
		return NULL;
	}

	unsigned int filepath_len = strlen(argv[1]);
	ret->path = (filepath) calloc(filepath_len, sizeof(*(ret->path)));
	if (ret->path == NULL) {
		free(ret);
		return NULL;
	}
	strcpy(ret->path, argv[1]);
	ret->comp = (comparator) argv[2][0];
	ret->mod_date = parse_date(argv[3]);

	return ret;
}

int main(int argc, char *argv[]) {
	global_criteria = parse_args(argc, argv);
	if (global_criteria == NULL) {
		return 1;
	}

	error_code = nftw_ls();
	if (error_code < 0) {
		printf("nftw ls failed. Exiting...\n");
		return 2;
	}
	printf("\n");

	free_criteria();

	return 0;
}
