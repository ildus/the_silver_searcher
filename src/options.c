#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "dropt.h"
#include "ignore.h"
#include "lang.h"
#include "log.h"
#include "options.h"
#include "print.h"
#include "util.h"

const char *color_line_number = "\033[1;33m"; /* bold yellow */
const char *color_match = "\033[30;43m";      /* black with yellow background */
const char *color_path = "\033[1;32m";        /* bold green */

cli_options opts;

void usage(dropt_context *ctx) {
    printf("\n");
    printf("Usage: ag [FILE-TYPE] [OPTIONS] PATTERN [PATH]\n\n");

    printf("  Recursively search for PATTERN in PATH.\n");
    printf("  Like grep or ack, but faster.\n\n");

    printf("Example:\n  ag -i foo /bar/\n\n");
    // dropt_print_help(stdout, ctx, NULL);

    printf("\
Output Options:\n\
     --ackmate            Print results in AckMate-parseable format\n\
  -A --after [LINES]      Print lines after match (Default: 2)\n\
  -B --before [LINES]     Print lines before match (Default: 2)\n\
     --[no]break          Print newlines between matches in different files\n\
                          (Enabled by default)\n\
  -c --count              Only print the number of matches in each file.\n\
                          (This often differs from the number of matching lines)\n\
     --[no]color          Print color codes in results (Enabled by default)\n\
     --color-line-number  Color codes for line numbers (Default: 1;33)\n\
     --color-match        Color codes for result match numbers (Default: 30;43)\n\
     --color-path         Color codes for path names (Default: 1;32)\n\
");
#ifdef _WIN32
    printf("\
     --color-win-ansi     Use ansi colors on Windows even where we can use native\n\
                          (pager/pipe colors are ansi regardless) (Default: off)\n\
");
#endif
    printf("\
     --column             Print column numbers in results\n\
     --[no]filename       Print file names (Enabled unless searching a single file)\n\
  -H --[no]heading        Print file names before each file's matches\n\
                          (Enabled by default)\n\
  -C --context [LINES]    Print lines before and after matches (Default: 2)\n\
     --[no]group          Same as --[no]break --[no]heading\n\
  -g --filename-pattern PATTERN\n\
                          Print filenames matching PATTERN\n\
  -l --files-with-matches Only print filenames that contain matches\n\
                          (don't print the matching lines)\n\
  -L --files-without-matches\n\
                          Only print filenames that don't contain matches\n\
     --print-all-files    Print headings for all files searched, even those that\n\
                          don't contain matches\n\
     --[no]numbers        Print line numbers. Default is to omit line numbers\n\
                          when searching streams\n\
  -o --only-matching      Prints only the matching part of the lines\n\
     --print-long-lines   Print matches on very long lines (Default: >2k characters)\n\
     --passthrough        When searching a stream, print all lines even if they\n\
                          don't match\n\
     --silent             Suppress all log messages, including errors\n\
     --stats              Print stats (files scanned, time taken, etc.)\n\
     --stats-only         Print stats and nothing else.\n\
                          (Same as --count when searching a single file)\n\
     --vimgrep            Print results like vim's :vimgrep /pattern/g would\n\
                          (it reports every match on the line)\n\
  -0 --null --print0      Separate filenames with null (for 'xargs -0')\n\
\n\
Search Options:\n\
  -a --all-types          Search all files (doesn't include hidden files\n\
                          or patterns from ignore files)\n\
  -D --debug              Ridiculous debugging (probably not useful)\n\
     --depth NUM          Search up to NUM directories deep (Default: 25)\n\
  -f --follow             Follow symlinks\n\
  -F --fixed-strings      Alias for --literal for compatibility with grep\n\
  -G --file-search-regex  PATTERN Limit search to filenames matching PATTERN\n\
     --hidden             Search hidden files (obeys .*ignore files)\n\
  -i --ignore-case        Match case insensitively\n\
     --ignore PATTERN     Ignore files/directories matching PATTERN\n\
                          (literal file/directory names also allowed)\n\
     --ignore-dir NAME    Alias for --ignore for compatibility with ack.\n\
  -m --max-count NUM      Skip the rest of a file after NUM matches (Default: 10,000)\n\
     --one-device         Don't follow links to other devices.\n\
  -p --path-to-ignore STRING\n\
                          Use .ignore file at STRING\n\
  -Q --literal            Don't parse PATTERN as a regular expression\n\
  -s --case-sensitive     Match case sensitively\n\
  -S --smart-case         Match case insensitively unless PATTERN contains\n\
                          uppercase characters (Enabled by default)\n\
     --search-binary      Search binary files for matches\n\
  -t --all-text           Search all text files (doesn't include hidden files)\n\
  -u --unrestricted       Search all files (ignore .ignore, .gitignore, etc.;\n\
                          searches binary and hidden files as well)\n\
  -U --skip-vcs-ignores   Ignore VCS ignore files\n\
                          (.gitignore, .hgignore; still obey .ignore)\n\
  -v --invert-match\n\
  -w --word-regexp        Only match whole words\n\
  -W --width NUM          Truncate match lines after NUM characters\n\
  -z --search-zip         Search contents of compressed (e.g., gzip) files\n\
\n");
    printf("File Types:\n\
The search can be restricted to certain types of files. Example:\n\
  ag --html needle\n\
  - Searches for 'needle' in files with suffix .htm, .html, .shtml or .xhtml.\n\
\n\
For a list of supported file types run:\n\
  ag --list-file-types\n\n\
ag was originally created by Geoff Greer. More information (and the latest release)\n\
can be found at http://geoff.greer.fm/ag\n");
}

void print_version(void) {
    printf("ag version (tre regexes) 0.1\n\n");
}

void init_options(void) {
    char *term = getenv("TERM");

    memset(&opts, 0, sizeof(opts));
    opts.casing = CASE_DEFAULT;
    opts.color = TRUE;
    if (term && !strcmp(term, "dumb")) {
        opts.color = FALSE;
    }
    opts.color_win_ansi = FALSE;
    opts.max_matches_per_file = 0;
    opts.max_search_depth = DEFAULT_MAX_SEARCH_DEPTH;
#if defined(__APPLE__) || defined(__MACH__)
    /* mamp() is slower than normal read() on macos. default to off */
    opts.mmap = FALSE;
#else
    opts.mmap = TRUE;
#endif
    opts.multiline = TRUE;
    opts.width = 0;
    opts.path_sep = '\n';
    opts.print_break = TRUE;
    opts.print_path = PATH_PRINT_DEFAULT;
    opts.print_all_paths = FALSE;
    opts.print_line_numbers = TRUE;
    opts.recurse_dirs = TRUE;
    opts.color_path = ag_strdup(color_path);
    opts.color_match = ag_strdup(color_match);
    opts.color_line_number = ag_strdup(color_line_number);
    opts.use_thread_affinity = TRUE;
}

void cleanup_options(void) {
    free(opts.color_path);
    free(opts.color_match);
    free(opts.color_line_number);

    if (opts.query) {
        free(opts.query);
    }

    free(opts.re);

    if (opts.ackmate_dir_filter) {
        free(opts.ackmate_dir_filter);
    }

    if (opts.file_search_regex) {
        free(opts.file_search_regex);
    }
}

void parse_options(int argc, char **argv, char **base_paths[], char **paths[]) {
    int ch;
    size_t i;
    int path_len = 0;
    int base_path_len = 0;
    dropt_bool useless = 0;
    dropt_uintptr group = 1;
    dropt_uintptr version = 0;
    int list_file_types = 0;
    int opt_index = 0;
    char *num_end;
    const char *home_dir = getenv("HOME");
    char *ignore_file_path = NULL;
    int accepts_query = 1;
    int needs_query = 1;
    struct stat statbuf;
    int rv;
    size_t lang_count;
    size_t lang_num = 0;
    int has_filetype = 0;

    size_t baseopts_len, full_len;
    dropt_option *all_options;
    char *lang_regex = NULL;
    size_t *ext_index = NULL;
    char *extensions = NULL;
    size_t num_exts = 0;

    dropt_bool case_sensitive = 0, opt_all_types = 0, show_help = 0, opt_count = 0,
               opt_debug = 0, opt_filename = 0, opt_nofilename = 0, opt_nopager = 0, opt_silent = 0,
               opt_stats_only = 0,
               opt_invert_match = 0, opt_unrestricted = 0, opt_files_with_matches = 0,
               opt_files_without_matches = 0;

    char *ackmate_dir_filter_str = NULL;
    char *ignore_dir_str = NULL;
    char *ignore_str = NULL;
    char *path_ignore_str = NULL;

    char *file_search_regex = NULL;
    char *file_search_regex_g = NULL;
    char *file_search_regex_G = NULL;

    char *color_match_str = NULL;
    char *color_path_str = NULL;
    char *color_line_number_str = NULL;

    init_options();

    dropt_option base_options[] = {
        { 'h', "help", "", NULL, dropt_handle_bool, &show_help },
        { 'A', "after", "", "N", dropt_handle_int, &opts.after },
        { 'C', "context", "", "N", dropt_handle_int, &opts.context },
        { 't', "all-text", "", NULL, dropt_handle_const, &opts.search_all_files, 0, 1 },
        { 'a', "all-types", "", NULL, dropt_handle_bool, &opt_all_types },
        { 'B', "before", "", "N", dropt_handle_int, &opts.before },

        { '\0', "break", "", NULL, dropt_handle_const, &opts.print_break, 0, 1 },
        { '\0', "no-break", "", NULL, dropt_handle_const, &opts.print_break, 0, 0 },
        { '\0', "nobreak", "", NULL, dropt_handle_const, &opts.print_break, 0, 0 },

        { '\0', "color", "", NULL, dropt_handle_const, &opts.color, 0, 1 },
        { '\0', "no-color", "", NULL, dropt_handle_const, &opts.color, 0, 0 },
        { '\0', "nocolor", "", NULL, dropt_handle_const, &opts.color, 0, 0 },
#ifdef _WIN32
        { '\0', "color-win-ansi", "", NULL, dropt_handle_const, &opts.color_win_ansi, 0, TRUE },
#endif
        { '\0', "search-binary", "", NULL, dropt_handle_const, &opts.search_binary_files, 0, 1 },
        { '\0', "search-files", "", NULL, dropt_handle_const, &opts.search_stream, 0, 1 },
        { 'z', "search-zip", "", NULL, dropt_handle_const, &opts.search_zip_files, 0, 1 },
        { '\0', "print-long-lines", "", NULL, dropt_handle_const, &opts.print_long_lines, 0, 1 },
        { '\0', "parallel", "", NULL, dropt_handle_const, &opts.parallel, 0, 1 },

        { '\0', "passthrough", "", NULL, dropt_handle_const, &opts.passthrough, 0, 1 },
        { '\0', "passthru", "", NULL, dropt_handle_const, &opts.passthrough, 0, 1 },

        { '\0', "column", "", NULL, dropt_handle_const, &opts.column, 0, 1 },
        { '\0', "hidden", "", NULL, dropt_handle_const, &opts.search_hidden_files, 0, 1 },
        { '\0', "list-file-types", "", NULL, dropt_handle_const, &list_file_types, 0, 1 },

        { '\0', "line-numbers", "", NULL, dropt_handle_const, &opts.print_line_numbers, 0, 2 },
        { '\0', "numbers", "", NULL, dropt_handle_const, &opts.print_line_numbers, 0, 2 },
        { '\0', "no-numbers", "", NULL, dropt_handle_const, &opts.print_line_numbers, 0, FALSE },
        { '\0', "nonumbers", "", NULL, dropt_handle_const, &opts.print_line_numbers, 0, FALSE },

        { '\0', "multiline", "", NULL, dropt_handle_const, &opts.multiline, 0, TRUE },
        { '\0', "no-multiline", "", NULL, dropt_handle_const, &opts.multiline, 0, FALSE },
        { '\0', "nomultiline", "", NULL, dropt_handle_const, &opts.multiline, 0, FALSE },

        { 'f', "follow", "", NULL, dropt_handle_const, &opts.follow_symlinks, 0, 1 },
        { '\0', "no-follow", "", NULL, dropt_handle_const, &opts.follow_symlinks, 0, 0 },
        { '\0', "nofollow", "", NULL, dropt_handle_const, &opts.follow_symlinks, 0, 0 },

        { 'H', "heading", "", NULL, dropt_handle_const, &opts.print_path, 0, PATH_PRINT_TOP },
        { '\0', "no-heading", "", NULL, dropt_handle_const, &opts.print_path, 0, PATH_PRINT_EACH_LINE },
        { '\0', "noheading", "", NULL, dropt_handle_const, &opts.print_path, 0, PATH_PRINT_EACH_LINE },

        { '\0', "group", "", NULL, dropt_handle_const, &group, 0, 1 },
        { '\0', "no-group", "", NULL, dropt_handle_const, &group, 0, 0 },
        { '\0', "nogroup", "", NULL, dropt_handle_const, &group, 0, 0 },

        { '\0', "mmap", "", NULL, dropt_handle_const, &opts.mmap, 0, TRUE },
        { '\0', "no-mmap", "", NULL, dropt_handle_const, &opts.mmap, 0, FALSE },
        { '\0', "nommap", "", NULL, dropt_handle_const, &opts.mmap, 0, FALSE },

        { '\0', "one-device", "", NULL, dropt_handle_const, &opts.one_dev, 0, 1 },
        { 'V', "version", "", NULL, dropt_handle_const, &version, 0, 1 },
        { '\0', "vimgrep", "", NULL, dropt_handle_const, &opts.vimgrep, 0, 1 },
        { '\0', "ackmate", "Print results in AckMate-parseable format", NULL, dropt_handle_const, &opts.ackmate, 0, 1 },

        { '\0', "affinity", "", NULL, dropt_handle_const, &opts.use_thread_affinity, 0, 1 },
        { '\0', "no-affinity", "", NULL, dropt_handle_const, &opts.use_thread_affinity, 0, 0 },
        { '\0', "noaffinity", "", NULL, dropt_handle_const, &opts.use_thread_affinity, 0, 0 },

        { 'c', "count", "", NULL, dropt_handle_bool, &opt_count },
        { 'D', "debug", "", NULL, dropt_handle_bool, &opt_debug },
        { 'l', "files-with-matches", "", NULL, dropt_handle_bool, &opt_files_with_matches },
        { 'L', "files-without-matches", "", NULL, dropt_handle_bool, &opt_files_without_matches },

        { '\0', "filename", "", NULL, dropt_handle_bool, &opt_filename },
        { '\0', "no-filename", "", NULL, dropt_handle_bool, &opt_nofilename },
        { '\0', "nofilename", "", NULL, dropt_handle_bool, &opt_nofilename },

        { '\0', "null", "", NULL, dropt_handle_const, &opts.path_sep, 0, '\0' },
        { 'o', "only-matching", "", NULL, dropt_handle_const, &opts.only_matching, 0, 1 },
        { '\0', "print-all-files", "", NULL, dropt_handle_const, &opts.print_all_paths, 0, TRUE },
        { '\0', "silent", "", NULL, dropt_handle_bool, &opt_silent },
        { 'U', "skip-vcs-ignores", "", NULL, dropt_handle_const, &opts.skip_vcs_ignores, 0, 1 },
        { '\0', "stats", "", NULL, dropt_handle_const, &opts.stats, 0, 1 },
        { '\0', "stats-only", "", NULL, dropt_handle_bool, &opt_stats_only },
        { 'u', "unrestricted", "", NULL, dropt_handle_bool, &opt_unrestricted },
        { 'w', "word-regexp", "", NULL, dropt_handle_const, &opts.word_regexp, 0, 1 },
        { 'v', "invert-match", "", NULL, dropt_handle_bool, &opt_invert_match },
        { '0', "print0", "", NULL, dropt_handle_const, &opts.path_sep, 0, '\0' },

        { 'R', NULL, "", NULL, dropt_handle_const, &opts.recurse_dirs, 0, 1 },
        { 'r', "recurse", "", NULL, dropt_handle_const, &opts.recurse_dirs, 0, 1 },
        { 'n', "no-recurse", "", NULL, dropt_handle_const, &opts.recurse_dirs, 0, 0 },
        { 'n', "norecurse", "", NULL, dropt_handle_const, &opts.recurse_dirs, 0, 0 },

        { 'Q', "literal", "", NULL, dropt_handle_const, &opts.literal, 0, 1 },
        { 'F', "fixed-strings", "", NULL, dropt_handle_const, &opts.literal, 0, 1 },

        { '\0', "match", "", NULL, dropt_handle_bool, &useless },

        { 's', "case-sensitive", "", NULL, dropt_handle_const, &opts.casing, 0, CASE_SENSITIVE },
        { 'S', "smart-case", "", NULL, dropt_handle_const, &opts.casing, 0, CASE_SMART },
        { 'i', "ignore-case", "", NULL, dropt_handle_const, &opts.casing, 0, CASE_INSENSITIVE },

        { '\0', "ackmate-dir-filter", "", "", dropt_handle_string, &ackmate_dir_filter_str },
        { '\0', "depth", "", "", dropt_handle_int, &opts.max_search_depth },
        { '\0', "workers", "", "", dropt_handle_int, &opts.workers },
        { 'W', "width", "", "", dropt_handle_int, &opts.width },
        { 'm', "max-count", "", "", dropt_handle_int, &opts.max_matches_per_file },

        { '\0', "ignore-dir", "", "", dropt_handle_string, &ignore_dir_str },
        { '\0', "ignore", "", "", dropt_handle_string, &ignore_str },
        { 'p', "path-to-ignore", "", "", dropt_handle_string, &path_ignore_str },

        { '\0', "pager", "", "", dropt_handle_string, &opts.pager },
        { '\0', "no-pager", "", NULL, dropt_handle_bool, &opt_nopager },
        { '\0', "nopager", "", NULL, dropt_handle_bool, &opt_nopager },

        { 'g', "filename-pattern", "", "", dropt_handle_string, &file_search_regex_g },
        { 'G', "file-search-regex", "", "", dropt_handle_string, &file_search_regex_G },
        { '\0', "color-line-number", "", "", dropt_handle_string, &color_line_number_str },
        { '\0', "color-match", "", "", dropt_handle_string, &color_match_str },
        { '\0', "color-path", "", "", dropt_handle_string, &color_path_str },
    };

    lang_count = get_lang_count();
    baseopts_len = (sizeof(base_options) / sizeof(dropt_option));
    full_len = (baseopts_len + lang_count + 1);
    all_options = ag_malloc(full_len * sizeof(dropt_option));
    memcpy(all_options, base_options, sizeof(base_options));
    ext_index = (size_t *)ag_malloc(sizeof(size_t) * lang_count);
    memset(ext_index, 0, sizeof(size_t) * lang_count);

    dropt_bool *opt_langs = (dropt_bool *)ag_malloc(lang_count * sizeof(dropt_bool));

    for (i = 0; i < lang_count; i++) {
        dropt_option opt = { '\0', langs[i].name, "", NULL, dropt_handle_bool, &opt_langs[i] };
        all_options[i + baseopts_len] = opt;
    }

    /* Required sentinel value. */
    all_options[full_len - 1] = (dropt_option){ '\0', 0 };
    dropt_context *args_context = dropt_new_context(all_options);

    if (args_context == NULL) {
        printf("out of memory\n");
        exit(1);
    }

    if (argc < 2) {
        usage(args_context);
        cleanup_ignore(root_ignores);
        cleanup_options();
        exit(1);
    }

    rv = fstat(fileno(stdin), &statbuf);
    if (rv == 0) {
        if (S_ISFIFO(statbuf.st_mode) || S_ISREG(statbuf.st_mode)) {
            opts.search_stream = 1;
        }
    }

    /* If we're not outputting to a terminal. change output to:
     * turn off colors
     * print filenames on every line
     */
    if (!isatty(fileno(stdout))) {
        opts.color = 0;
        group = 0;

        /* Don't search the file that stdout is redirected to */
        rv = fstat(fileno(stdout), &statbuf);
        if (rv != 0) {
            die("Error fstat()ing stdout");
        }
#ifdef __unix__
        opts.stdout_inode = statbuf.st_ino;
#endif
    }

    if (argc < 2) {
        usage(args_context);
        cleanup_ignore(root_ignores);
        cleanup_options();
        exit(1);
    } else {
        char **temp;

        argv = dropt_parse(args_context, -1, &argv[1]);
        if (dropt_get_error(args_context) != dropt_error_none) {
            fprintf(stderr, "ag: %s\n", dropt_get_error_message(args_context));
            exit(1);
        }

        temp = argv;
        argc = 0;

        while (*temp) {
            argc++;
            temp++;
        }
    }

    /* handle options */
    if (show_help) {
        usage(args_context);
        exit(0);
    }
    if (opt_all_types) {
        opts.search_all_files = 1;
        opts.search_binary_files = 1;
    }

    if (opt_count) {
        opts.print_count = 1;
        opts.print_filename_only = 1;
    }

    if (opt_debug)
        set_log_level(LOG_LEVEL_DEBUG);

    if (ackmate_dir_filter_str)
        compile_study(&opts.ackmate_dir_filter, ackmate_dir_filter_str, REG_EXTENDED);

    if (opt_filename) {
        opts.print_path = PATH_PRINT_DEFAULT;
        opts.print_line_numbers = TRUE;
    }

    if (ignore_dir_str) {
        add_ignore_pattern(root_ignores, ignore_dir_str);
    }

    if (ignore_str)
        add_ignore_pattern(root_ignores, ignore_str);

    if (path_ignore_str) {
        opts.path_to_ignore = TRUE;
        load_ignore_patterns(root_ignores, path_ignore_str);
    }

    if (opt_nofilename) {
        opts.print_path = PATH_PRINT_NOTHING;
        opts.print_line_numbers = FALSE;
    }

    if (opt_nopager) {
        out_fd = stdout;
        opts.pager = NULL;
    }

    if (opt_silent)
        set_log_level(LOG_LEVEL_NONE);

    if (opt_stats_only) {
        opts.print_filename_only = 1;
        opts.print_path = PATH_PRINT_NOTHING;
        opts.stats = 1;
    }

    if (opt_invert_match) {
        opts.invert_match = 1;
        /* Color highlighting doesn't make sense when inverting matches */
        opts.color = 0;
    }

    if (opt_unrestricted) {
        opts.search_binary_files = 1;
        opts.search_all_files = 1;
        opts.search_hidden_files = 1;
    }

    if (opt_files_with_matches) {
        needs_query = 0;
        opts.print_filename_only = 1;
        opts.print_path = PATH_PRINT_TOP;
    }

    if (opt_files_without_matches) {
        opts.print_nonmatching_files = 1;
        opts.print_path = PATH_PRINT_TOP;
    }

    if (file_search_regex_g) {
        needs_query = accepts_query = 0;
        opts.match_files = 1;
        file_search_regex_G = file_search_regex_g;
    }

    if (file_search_regex_G) {
        if (file_search_regex) {
            log_err("File search regex (-g or -G) already specified.");
            usage(args_context);
            exit(1);
        }

        file_search_regex = ag_strdup(file_search_regex_G);
    }

    if (color_line_number_str) {
        free(opts.color_line_number);
        ag_asprintf(&opts.color_line_number, "\033[%sm", color_line_number_str);
    }

    if (color_match_str) {
        free(opts.color_match);
        ag_asprintf(&opts.color_match, "\033[%sm", color_match_str);
    }

    if (color_path_str) {
        free(opts.color_path);
        ag_asprintf(&opts.color_path, "\033[%sm", color_path_str);
    }

    for (i = 0; i < lang_count; i++) {
        if (opt_langs[i]) {
            has_filetype = 1;
            ext_index[lang_num++] = i;
            break;
        }
    }

    if (version) {
        print_version();
        exit(0);
    }

    if (opts.casing == CASE_DEFAULT) {
        opts.casing = CASE_SMART;
    }

    if (file_search_regex) {
        int pcre_opts = REG_EXTENDED;
        if (opts.casing == CASE_INSENSITIVE || (opts.casing == CASE_SMART && is_lowercase(file_search_regex))) {
            pcre_opts |= REG_ICASE;
        }
        if (opts.word_regexp) {
            char *old_file_search_regex = file_search_regex;
            ag_asprintf(&file_search_regex, "\\b%s\\b", file_search_regex);
            free(old_file_search_regex);
        }
        compile_study(&opts.file_search_regex, file_search_regex, pcre_opts);
        free(file_search_regex);
    }

    if (has_filetype) {
        num_exts = combine_file_extensions(ext_index, lang_num, &extensions);
        lang_regex = make_lang_regex(extensions, num_exts);
        compile_study(&opts.file_search_regex, lang_regex, REG_EXTENDED);
    }

    if (extensions) {
        free(extensions);
    }
    free(ext_index);
    if (lang_regex) {
        free(lang_regex);
    }
    free(all_options);
    free(opt_langs);

    if (opts.pager) {
        out_fd = popen(opts.pager, "w");
        if (!out_fd) {
            perror("Failed to run pager");
            exit(1);
        }
    }

#ifdef HAVE_PLEDGE
    if (opts.skip_vcs_ignores) {
        if (pledge("stdio rpath proc", NULL) == -1) {
            die("pledge: %s", strerror(errno));
        }
    }
#endif

    if (list_file_types) {
        size_t lang_index;
        printf("The following file types are supported:\n");
        for (lang_index = 0; lang_index < lang_count; lang_index++) {
            printf("  --%s\n    ", langs[lang_index].name);
            int j;
            for (j = 0; j < MAX_EXTENSIONS && langs[lang_index].extensions[j]; j++) {
                printf("  .%s", langs[lang_index].extensions[j]);
            }
            printf("\n\n");
        }
        exit(0);
    }

    if (needs_query && argc == 0) {
        log_err("What do you want to search for?");
        exit(1);
    }

    if (home_dir && !opts.search_all_files) {
        log_debug("Found user's home dir: %s", home_dir);
        ag_asprintf(&ignore_file_path, "%s/.agignore", home_dir);
        load_ignore_patterns(root_ignores, ignore_file_path);
        free(ignore_file_path);
    }

    if (!opts.skip_vcs_ignores) {
        FILE *gitconfig_file = NULL;
        size_t buf_len = 0;
        char *gitconfig_res = NULL;

#ifdef _WIN32
        gitconfig_file = popen("git config -z --path --get core.excludesfile 2>NUL", "r");
#else
        gitconfig_file = popen("git config -z --path --get core.excludesfile 2>/dev/null", "r");
#endif
        if (gitconfig_file != NULL) {
            do {
                gitconfig_res = ag_realloc(gitconfig_res, buf_len + 65);
                buf_len += fread(gitconfig_res + buf_len, 1, 64, gitconfig_file);
            } while (!feof(gitconfig_file) && buf_len > 0 && buf_len % 64 == 0);
            gitconfig_res[buf_len] = '\0';
            if (buf_len == 0) {
                free(gitconfig_res);
                const char *config_home = getenv("XDG_CONFIG_HOME");
                if (config_home) {
                    ag_asprintf(&gitconfig_res, "%s/%s", config_home, "git/ignore");
                } else if (home_dir) {
                    ag_asprintf(&gitconfig_res, "%s/%s", home_dir, ".config/git/ignore");
                } else {
                    gitconfig_res = ag_strdup("");
                }
            }
            log_debug("global core.excludesfile: %s", gitconfig_res);
            load_ignore_patterns(root_ignores, gitconfig_res);
            free(gitconfig_res);
            pclose(gitconfig_file);
        }
    }

#ifdef HAVE_PLEDGE
    if (pledge("stdio rpath proc", NULL) == -1) {
        die("pledge: %s", strerror(errno));
    }
#endif

    if (opts.context > 0) {
        opts.before = opts.context;
        opts.after = opts.context;
    }

    if (opts.ackmate) {
        opts.color = 0;
        opts.print_break = 1;
        group = 1;
        opts.search_stream = 0;
    }

    if (opts.vimgrep) {
        opts.color = 0;
        opts.print_break = 0;
        group = 1;
        opts.search_stream = 0;
        opts.print_path = PATH_PRINT_NOTHING;
    }

    if (opts.parallel) {
        opts.search_stream = 0;
    }

    if (!(opts.print_path != PATH_PRINT_DEFAULT || opts.print_break == 0)) {
        if (group) {
            opts.print_break = 1;
        } else {
            opts.print_path = PATH_PRINT_DEFAULT_EACH_LINE;
            opts.print_break = 0;
        }
    }

    if (opts.search_stream) {
        opts.print_break = 0;
        opts.print_path = PATH_PRINT_NOTHING;
        if (opts.print_line_numbers != 2) {
            opts.print_line_numbers = 0;
        }
    }

    if (accepts_query && argc > 0) {
        if (!needs_query && strlen(argv[0]) == 0) {
            // use default query
            opts.query = ag_strdup(".");
        } else {
            // use the provided query
            opts.query = ag_strdup(argv[0]);
        }
        argc--;
        argv++;
    } else if (!needs_query) {
        // use default query
        opts.query = ag_strdup(".");
    }
    opts.query_len = strlen(opts.query);

    log_debug("Query is %s", opts.query);

    if (opts.query_len == 0) {
        log_err("Error: No query. What do you want to search for?");
        exit(1);
    }

    if (!is_regex(opts.query)) {
        opts.literal = 1;
    }

    char *path = NULL;
    char *base_path = NULL;
#ifdef PATH_MAX
    char *tmp = NULL;
#endif
    opts.paths_len = argc;
    if (argc > 0) {
        *paths = ag_calloc(sizeof(char *), argc + 1);
        *base_paths = ag_calloc(sizeof(char *), argc + 1);
        for (i = 0; i < (size_t)argc; i++) {
            path = ag_strdup(argv[i]);
            path_len = strlen(path);
            /* kill trailing slash */
            if (path_len > 1 && path[path_len - 1] == '/') {
                path[path_len - 1] = '\0';
            }
            (*paths)[i] = path;
#ifdef PATH_MAX
            tmp = ag_malloc(PATH_MAX);
            base_path = realpath(path, tmp);
#else
            base_path = realpath(path, NULL);
#endif
            if (base_path) {
                base_path_len = strlen(base_path);
                /* add trailing slash */
                if (base_path_len > 1 && base_path[base_path_len - 1] != '/') {
                    base_path = ag_realloc(base_path, base_path_len + 2);
                    base_path[base_path_len] = '/';
                    base_path[base_path_len + 1] = '\0';
                }
            }
            (*base_paths)[i] = base_path;
        }
        /* Make sure we search these paths instead of stdin. */
        opts.search_stream = 0;
    } else {
        path = ag_strdup(".");
        *paths = ag_malloc(sizeof(char *) * 2);
        *base_paths = ag_malloc(sizeof(char *) * 2);
        (*paths)[0] = path;
#ifdef PATH_MAX
        tmp = ag_malloc(PATH_MAX);
        (*base_paths)[0] = realpath(path, tmp);
#else
        (*base_paths)[0] = realpath(path, NULL);
#endif
        i = 1;
    }
    (*paths)[i] = NULL;
    (*base_paths)[i] = NULL;

#ifdef _WIN32
    windows_use_ansi(opts.color_win_ansi);
#endif
}
