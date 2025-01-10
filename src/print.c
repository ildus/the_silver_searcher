#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ignore.h"
#include "log.h"
#include "options.h"
#include "print.h"
#include "search.h"
#include "util.h"
#ifdef _WIN32
#define fprintf(...) fprintf_w32(__VA_ARGS__)
#endif

int first_file_match = 1;

const char *color_reset = "\033[0m\033[K";

const char *truncate_marker = " [...]";

print_context_t *print_init_context(void) {
    print_context_t *ctx = ag_malloc(sizeof(print_context_t));

    ctx->context_prev_lines = ag_calloc(sizeof(char *), (opts.before + 1));
    ctx->line = 1;
    ctx->prev_line = 0;
    ctx->last_prev_line = 0;
    ctx->prev_line_offset = 0;
    ctx->line_preceding_current_match_offset = 0;
    ctx->lines_since_last_match = INT_MAX;
    ctx->last_printed_match = 0;
    ctx->in_a_match = FALSE;
    ctx->printing_a_match = FALSE;

    return ctx;
}

void print_cleanup_context(print_context_t *ctx) {
    size_t i;

    if (!ctx || ctx->context_prev_lines == NULL) {
        return;
    }

    for (i = 0; i < opts.before; i++) {
        if (ctx->context_prev_lines[i] != NULL) {
            free(ctx->context_prev_lines[i]);
        }
    }
    free(ctx->context_prev_lines);
    ctx->context_prev_lines = NULL;
    free(ctx);
}

void print_context_append(print_context_t *ctx, const char *line, size_t len) {
    if (opts.before == 0) {
        return;
    }
    if (ctx->context_prev_lines[ctx->last_prev_line] != NULL) {
        free(ctx->context_prev_lines[ctx->last_prev_line]);
    }
    ctx->context_prev_lines[ctx->last_prev_line] = ag_strndup(line, len);
    ctx->last_prev_line = (ctx->last_prev_line + 1) % opts.before;
}

void print_trailing_context(print_context_t *ctx, const char *path, const char *buf, size_t n) {
    char sep = '-';

    if (opts.ackmate || opts.vimgrep) {
        sep = ':';
    }

    if (ctx->lines_since_last_match != 0 &&
        ctx->lines_since_last_match <= opts.after) {
        if (opts.print_path == PATH_PRINT_EACH_LINE) {
            print_path(path, ':');
        }
        print_line_number(ctx->line, sep);

        fwrite(buf, 1, n, out_fd);
        fputc('\n', out_fd);
    }

    ctx->line++;
    if (!ctx->in_a_match && ctx->lines_since_last_match < INT_MAX) {
        ctx->lines_since_last_match++;
    }
}

void print_path(const char *path, const char sep) {
    if (opts.print_path == PATH_PRINT_NOTHING && !opts.vimgrep) {
        return;
    }
    path = normalize_path(path);

    if (opts.ackmate) {
        fprintf(out_fd, ":%s%c", path, sep);
    } else if (opts.vimgrep) {
        fprintf(out_fd, "%s%c", path, sep);
    } else {
        if (opts.color) {
            fprintf(out_fd, "%s%s%s%c", opts.color_path, path, color_reset, sep);
        } else {
            fprintf(out_fd, "%s%c", path, sep);
        }
    }
}

void print_path_count(const char *path, const char sep, const size_t count) {
    if (*path) {
        print_path(path, ':');
    }
    if (opts.color) {
        fprintf(out_fd, "%s%lu%s%c", opts.color_line_number, (unsigned long)count, color_reset, sep);
    } else {
        fprintf(out_fd, "%lu%c", (unsigned long)count, sep);
    }
}

void print_line(const char *buf, size_t buf_pos, size_t prev_line_offset) {
    size_t write_chars = buf_pos - prev_line_offset + 1;
    if (opts.width > 0 && opts.width < write_chars) {
        write_chars = opts.width;
    }

    fwrite(buf + prev_line_offset, 1, write_chars, out_fd);
}

void print_binary_file_matches(const char *path) {
    path = normalize_path(path);
    print_file_separator();
    fprintf(out_fd, "Binary file %s matches.\n", path);
}

void print_file_matches(print_context_t *ctx, const char *path, const char *buf, const size_t buf_len, const match_t matches[], const size_t matches_len) {
    size_t cur_match = 0;
    ssize_t lines_to_print = 0;
    char sep = '-';
    size_t i, j;
    int blanks_between_matches = opts.context || opts.after || opts.before;

    if (opts.ackmate || opts.vimgrep) {
        sep = ':';
    }

    print_file_separator();

    if (opts.print_path == PATH_PRINT_DEFAULT) {
        opts.print_path = PATH_PRINT_TOP;
    } else if (opts.print_path == PATH_PRINT_DEFAULT_EACH_LINE) {
        opts.print_path = PATH_PRINT_EACH_LINE;
    }

    if (opts.print_path == PATH_PRINT_TOP) {
        if (opts.print_count) {
            print_path_count(path, opts.path_sep, matches_len);
        } else {
            print_path(path, opts.path_sep);
        }
    }

    for (i = 0; i <= buf_len && (cur_match < matches_len || ctx->lines_since_last_match <= opts.after); i++) {
        if (cur_match < matches_len && i == matches[cur_match].start) {
            ctx->in_a_match = TRUE;
            /* We found the start of a match */
            if (cur_match > 0 && blanks_between_matches && ctx->lines_since_last_match > (opts.before + opts.after + 1)) {
                fprintf(out_fd, "--\n");
            }

            if (ctx->lines_since_last_match > 0 && opts.before > 0) {
                /* TODO: better, but still needs work */
                /* print the previous line(s) */
                lines_to_print = ctx->lines_since_last_match - (opts.after + 1);
                if (lines_to_print < 0) {
                    lines_to_print = 0;
                } else if ((size_t)lines_to_print > opts.before) {
                    lines_to_print = opts.before;
                }

                for (j = (opts.before - lines_to_print); j < opts.before; j++) {
                    ctx->prev_line = (ctx->last_prev_line + j) % opts.before;
                    if (ctx->context_prev_lines[ctx->prev_line] != NULL) {
                        if (opts.print_path == PATH_PRINT_EACH_LINE) {
                            print_path(path, ':');
                        }
                        print_line_number(ctx->line - (opts.before - j), sep);
                        fprintf(out_fd, "%s\n", ctx->context_prev_lines[ctx->prev_line]);
                    }
                }
            }
            ctx->lines_since_last_match = 0;
        }

        if (cur_match < matches_len && i == matches[cur_match].end) {
            /* We found the end of a match. */
            cur_match++;
            ctx->in_a_match = FALSE;
        }

        /* We found the end of a line. */
        if ((i == buf_len || buf[i] == '\n') && opts.before > 0) {
            /* We don't want to strcpy the \n */
            print_context_append(ctx, &buf[ctx->prev_line_offset], i - ctx->prev_line_offset);
        }

        if (i == buf_len || buf[i] == '\n') {
            if (ctx->lines_since_last_match == 0) {
                if (opts.print_path == PATH_PRINT_EACH_LINE && !opts.search_stream) {
                    print_path(path, ':');
                }
                if (opts.ackmate) {
                    /* print headers for ackmate to parse */
                    print_line_number(ctx->line, ';');
                    for (; ctx->last_printed_match < cur_match; ctx->last_printed_match++) {
                        size_t start = matches[ctx->last_printed_match].start - ctx->line_preceding_current_match_offset;
                        fprintf(out_fd, "%lu %lu",
                                start,
                                matches[ctx->last_printed_match].end - matches[ctx->last_printed_match].start);
                        ctx->last_printed_match == cur_match - 1 ? fputc(':', out_fd) : fputc(',', out_fd);
                    }
                    print_line(buf, i, ctx->prev_line_offset);
                } else if (opts.vimgrep) {
                    for (; ctx->last_printed_match < cur_match; ctx->last_printed_match++) {
                        print_path(path, sep);
                        print_line_number(ctx->line, sep);
                        print_column_number(matches, ctx->last_printed_match, ctx->prev_line_offset, sep);
                        print_line(buf, i, ctx->prev_line_offset);
                    }
                } else {
                    print_line_number(ctx->line, ':');
                    int printed_match = FALSE;
                    if (opts.column) {
                        print_column_number(matches, ctx->last_printed_match, ctx->prev_line_offset, ':');
                    }

                    if (ctx->printing_a_match && opts.color) {
                        fprintf(out_fd, "%s", opts.color_match);
                    }
                    for (j = ctx->prev_line_offset; j <= i; j++) {
                        /* close highlight of match term */
                        if (ctx->last_printed_match < matches_len && j == matches[ctx->last_printed_match].end) {
                            if (opts.color) {
                                fprintf(out_fd, "%s", color_reset);
                            }
                            ctx->printing_a_match = FALSE;
                            ctx->last_printed_match++;
                            printed_match = TRUE;
                            if (opts.only_matching) {
                                fputc('\n', out_fd);
                            }
                        }
                        /* skip remaining characters if truncation width exceeded, needs to be done
                         * before highlight opening */
                        if (j < buf_len && opts.width > 0 && j - ctx->prev_line_offset >= opts.width) {
                            if (j < i) {
                                fputs(truncate_marker, out_fd);
                            }
                            fputc('\n', out_fd);

                            /* prevent any more characters or highlights */
                            j = i;
                            ctx->last_printed_match = matches_len;
                        }
                        /* open highlight of match term */
                        if (ctx->last_printed_match < matches_len && j == matches[ctx->last_printed_match].start) {
                            if (opts.only_matching && printed_match) {
                                if (opts.print_path == PATH_PRINT_EACH_LINE) {
                                    print_path(path, ':');
                                }
                                print_line_number(ctx->line, ':');
                                if (opts.column) {
                                    print_column_number(matches, ctx->last_printed_match, ctx->prev_line_offset, ':');
                                }
                            }
                            if (opts.color) {
                                fprintf(out_fd, "%s", opts.color_match);
                            }
                            ctx->printing_a_match = TRUE;
                        }
                        /* Don't print the null terminator */
                        if (j < buf_len) {
                            /* if only_matching is set, print only matches and newlines */
                            if (!opts.only_matching || ctx->printing_a_match) {
                                if (opts.width == 0 || j - ctx->prev_line_offset < opts.width) {
                                    fputc(buf[j], out_fd);
                                }
                            }
                        }
                    }
                    if (ctx->printing_a_match && opts.color) {
                        fprintf(out_fd, "%s", color_reset);
                    }
                }
            }

            if (opts.search_stream) {
                ctx->last_printed_match = 0;
                break;
            }

            /* print context after matching line */
            print_trailing_context(ctx, path, &buf[ctx->prev_line_offset], i - ctx->prev_line_offset);

            ctx->prev_line_offset = i + 1; /* skip the newline */
            if (!ctx->in_a_match) {
                ctx->line_preceding_current_match_offset = i + 1;
            }

            /* File doesn't end with a newline. Print one so the output is pretty. */
            if (i == buf_len && buf[i - 1] != '\n') {
                fputc('\n', out_fd);
            }
        }
    }
#ifdef __unix__
    /* Flush output if stdout is not a tty */
    if (opts.stdout_inode) {
        fflush(out_fd);
    }
#endif
}

void print_line_number(size_t line, const char sep) {
    if (!opts.print_line_numbers) {
        return;
    }
    if (opts.color) {
        fprintf(out_fd, "%s%lu%s%c", opts.color_line_number, (unsigned long)line, color_reset, sep);
    } else {
        fprintf(out_fd, "%lu%c", (unsigned long)line, sep);
    }
}

void print_column_number(const match_t matches[], size_t last_printed_match,
                         size_t prev_line_offset, const char sep) {
    size_t column = 0;
    if (prev_line_offset <= matches[last_printed_match].start) {
        column = (matches[last_printed_match].start - prev_line_offset) + 1;
    }
    fprintf(out_fd, "%lu%c", (unsigned long)column, sep);
}

void print_file_separator(void) {
    if (first_file_match == 0 && opts.print_break) {
        fprintf(out_fd, "\n");
    }
    first_file_match = 0;
}

const char *normalize_path(const char *path) {
    if (strlen(path) < 3) {
        return path;
    }
    if (path[0] == '.' && path[1] == '/') {
        return path + 2;
    }
    if (path[0] == '/' && path[1] == '/') {
        return path + 1;
    }
    return path;
}
