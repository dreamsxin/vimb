/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "config.h"
#include <stdio.h>
#include "ctype.h"
#include "util.h"

char *util_get_config_dir(void)
{
    char *path = g_build_filename(g_get_user_config_dir(), PROJECT, NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

char *util_get_cache_dir(void)
{
    char *path = g_build_filename(g_get_user_cache_dir(), PROJECT, NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

const char *util_get_home_dir(void)
{
    const char *dir = g_getenv("HOME");

    if (!dir) {
        dir = g_get_home_dir();
    }

    return dir;
}

void util_create_dir_if_not_exists(const char *dirpath)
{
    if (!g_file_test(dirpath, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(dirpath, 0755);
    }
}

void util_create_file_if_not_exists(const char *filename)
{
    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
        FILE *f = fopen(filename, "a");
        fclose(f);
    }
}

/**
 * Retrieves the length bytes from given file.
 *
 * The memory of returned string have to be freed!
 */
char *util_get_file_contents(const char *filename, gsize *length)
{
    GError *error  = NULL;
    char *content = NULL;
    if (!(g_file_test(filename, G_FILE_TEST_IS_REGULAR)
        && g_file_get_contents(filename, &content, length, &error))
    ) {
        fprintf(stderr, "Cannot open %s: %s\n", filename, error ? error->message : "file not found");
        g_clear_error(&error);
    }
    return content;
}

/**
 * Retrieves the file content as lines.
 *
 * The result have to be freed by g_strfreev().
 */
char **util_get_lines(const char *filename)
{
    char *content = util_get_file_contents(filename, NULL);
    char **lines  = NULL;
    if (content) {
        /* split the file content into lines */
        lines = g_strsplit(content, "\n", -1);
        g_free(content);
    }
    return lines;
}

/**
 * Retrieves a list with unique items from file.
 *
 * @filename:    file to read items from
 * @func:        function to parse a single line to item
 * @unique_func: function to decide if two items are equal
 * @free_func:   function to free already converted item if this isn't unque
 * @max_items:   maximum number of items that are returned, use 0 for
 *               unlimited items
 */
GList *util_file_to_unique_list(const char *filename, Util_Content_Func func,
    GCompareFunc unique_func, GDestroyNotify free_func, unsigned int max_items)
{
    GList *gl = NULL;
    /* yes, the whole file is read and wen possible don not need all the
     * lines, but this is easier to implement compared to reading the file
     * line wise from ent to begining */
    char *line, **lines = util_get_lines(filename);
    void *value;
    int len, num_items = 0;

    len = g_strv_length(lines);
    if (!len) {
        return gl;
    }

    /* begin with tha last line of the file to make unique check easier -
     * every already existing item in the list is the latest, so we don't need
     * to romove items from the list which takes some time */
    for (int i = len - 1; i >= 0; i--) {
        line = lines[i];
        g_strstrip(line);
        if (!*line) {
            continue;
        }

        if ((value = func(line))) {
            /* if the value is already in list, free it and don't put it onto
             * the list */
            if (g_list_find_custom(gl, value, unique_func)) {
                free_func(value);
            } else {
                gl = g_list_prepend(gl, value);
                /* skip the loop if we precessed max_items unique items */
                if (++num_items >= max_items) {
                    break;
                }
            }
        }
    }
    g_strfreev(lines);

    return gl;
}

/**
 * Append new data to file.
 *
 * @file:   File to appen the data
 * @format: Format string used to process va_list
 */
gboolean util_file_append(const char *file, const char *format, ...)
{
    va_list args;
    FILE *f;

    if ((f = fopen(file, "a+"))) {
        FILE_LOCK_SET(fileno(f), F_WRLCK);

        va_start(args, format);
        vfprintf(f, format, args);
        va_end(args);

        FILE_LOCK_SET(fileno(f), F_UNLCK);
        fclose(f);

        return true;
    }
    return false;
}

/**
 * Prepend new data to file.
 *
 * @file:   File to prepend the data
 * @format: Format string used to process va_list
 */
gboolean util_file_prepend(const char *file, const char *format, ...)
{
    gboolean res = false;
    va_list args;
    char *content;
    FILE *f;

    content = util_get_file_contents(file, NULL);
    if ((f = fopen(file, "w"))) {
        FILE_LOCK_SET(fileno(f), F_WRLCK);

        va_start(args, format);
        /* write new content to the file */
        vfprintf(f, format, args);
        va_end(args);

        /* append previous file content */
        fputs(content, f);

        FILE_LOCK_SET(fileno(f), F_UNLCK);
        fclose(f);

        res = true;
    }
    g_free(content);

    return res;
}

char *util_strcasestr(const char *haystack, const char *needle)
{
    guchar c1, c2;
    int i, j;
    int nlen = strlen(needle);
    int hlen = strlen(haystack) - nlen + 1;

    for (i = 0; i < hlen; i++) {
        for (j = 0; j < nlen; j++) {
            c1 = haystack[i + j];
            c2 = needle[j];
            if (toupper(c1) != toupper(c2)) {
                goto next;
            }
        }
        return (char*)haystack + i;
next:
        ;
    }
    return NULL;
}

/**
 * Replaces appearances of search in string by given replace.
 * Returne a new allocated string of search was found.
 */
char *util_str_replace(const char* search, const char* replace, const char* string)
{
    if (!string) {
        return NULL;
    }

    char **buf = g_strsplit(string, search, -1);
    char *ret  = g_strjoinv(replace, buf);
    g_strfreev(buf);

    return ret;
}

/**
 * Creates a temporary file with given content.
 *
 * Upon success, and if file is non-NULL, the actual file path used is
 * returned in file. This string should be freed with g_free() when not
 * needed any longer.
 */
gboolean util_create_tmp_file(const char *content, char **file)
{
    int fp;
    ssize_t bytes, len;

    fp = g_file_open_tmp(PROJECT "-XXXXXX", file, NULL);
    if (fp == -1) {
        fprintf(stderr, "Could not create temporary file %s", *file);
        g_free(*file);
        return false;
    }

    len = strlen(content);

    /* write content into temporary file */
    bytes = write(fp, content, len);
    if (bytes < len) {
        close(fp);
        unlink(*file);
        fprintf(stderr, "Could not write temporary file %s", *file);
        g_free(*file);

        return false;
    }
    close(fp);

    return true;
}

/**
 * Build the absolute file path of given path and possible given directory.
 * If the path is already absolute or uses ~/ for the home directory, the
 * directory is ignored.
 *
 * Returned path must be freed.
 */
char *util_build_path(const char *path, const char *dir)
{
    char *fullPath, *p;

    /* creating directory */
    if (path[0] == '/') {
        fullPath = g_strdup(path);
    } else if (path[0] == '~') {
        if (path[1] == '/') {
            fullPath = g_strconcat(g_get_home_dir(), &path[1], NULL);
        } else {
            fullPath = g_strconcat(g_get_home_dir(), "/", &path[1], NULL);
        }
    } else if (dir) {
        fullPath = g_strconcat(dir, "/", path, NULL);
    } else {
        fullPath = g_strconcat(g_get_current_dir(), "/", path, NULL);
    }

    if ((p = strrchr(fullPath, '/'))) {
        *p = '\0';
        g_mkdir_with_parents(fullPath, 0700);
        *p = '/';
    }

    return fullPath;
}
