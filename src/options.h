#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>
#include <sys/stat.h>

#include "dropt.h"
#include "tre.h"

#define DEFAULT_AFTER_LEN 2
#define DEFAULT_BEFORE_LEN 2
#define DEFAULT_CONTEXT_LEN 2
#define DEFAULT_MAX_SEARCH_DEPTH 25
enum case_behavior {
    CASE_DEFAULT, /* Changes to CASE_SMART at the end of option parsing */
    CASE_SENSITIVE,
    CASE_INSENSITIVE,
    CASE_SMART,
    CASE_SENSITIVE_RETRY_INSENSITIVE /* for future use */
};

enum path_print_behavior {
    PATH_PRINT_DEFAULT,           /* PRINT_TOP if > 1 file being searched, else PRINT_NOTHING */
    PATH_PRINT_DEFAULT_EACH_LINE, /* PRINT_EACH_LINE if > 1 file being searched, else PRINT_NOTHING */
    PATH_PRINT_TOP,
    PATH_PRINT_EACH_LINE,
    PATH_PRINT_NOTHING
};

typedef struct {
    dropt_uintptr ackmate;
    regex_t *ackmate_dir_filter;
    size_t after;
    size_t before;
    enum case_behavior casing;
    const char *file_search_string;
    dropt_uintptr match_files;
    regex_t *file_search_regex;
    dropt_uintptr color;
    char *color_line_number;
    char *color_match;
    char *color_path;
    dropt_uintptr color_win_ansi;
    dropt_uintptr column;
    size_t context;
    dropt_uintptr follow_symlinks;
    dropt_uintptr invert_match;
    dropt_uintptr literal;
    dropt_uintptr literal_starts_wordchar;
    dropt_uintptr literal_ends_wordchar;
    size_t max_matches_per_file;
    int max_search_depth;
    dropt_uintptr mmap;
    dropt_uintptr multiline;
    dropt_uintptr one_dev;
    dropt_uintptr only_matching;
    char path_sep;
    dropt_uintptr path_to_ignore;
    dropt_uintptr print_break;
    dropt_uintptr print_count;
    dropt_uintptr print_filename_only;
    dropt_uintptr print_nonmatching_files;
    dropt_uintptr print_path;
    dropt_uintptr print_all_paths;
    dropt_uintptr print_line_numbers;
    dropt_uintptr print_long_lines; /* TODO: support this in print.c */
    dropt_uintptr passthrough;
    regex_t *re;
    dropt_uintptr recurse_dirs;
    dropt_uintptr search_all_files;
    dropt_uintptr skip_vcs_ignores;
    dropt_uintptr search_binary_files;
    dropt_uintptr search_zip_files;
    dropt_uintptr search_hidden_files;
    dropt_uintptr search_stream; /* true if tail -F blah | ag */
    dropt_uintptr stats;
    size_t stream_line_num;    /* This should totally not be in here */
    dropt_uintptr match_found; /* This should totally not be in here */
    char *query;
    dropt_uintptr query_len;
    char *pager;
    dropt_uintptr paths_len;
    dropt_uintptr parallel;
    dropt_uintptr use_thread_affinity;
    dropt_uintptr vimgrep;
    size_t width;
    dropt_uintptr word_regexp;
    int workers;
#ifdef __unix__
    ino_t stdout_inode;
#endif
} cli_options;

/* global options. parse_options gives it sane values, everything else reads from it */
extern cli_options opts;

typedef struct option option_t;

void usage(dropt_context *);
void print_version(void);

void init_options(void);
void parse_options(int argc, char **argv, char **base_paths[], char **paths[]);
void cleanup_options(void);

#endif
