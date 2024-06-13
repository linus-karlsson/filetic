#include "logging.h"
#include "platform/platform.h"
#include <stdlib.h>
#include <string.h>

char* concatinate(const char* first, const size_t first_length,
                  const char* second, const size_t second_length,
                  const char delim_between, const size_t extra_length,
                  size_t* result_length)
{
    const size_t extra = delim_between != '\0';
    const size_t length = first_length + second_length + extra;
    char* result = (char*)calloc(length + 1 + extra_length, sizeof(char));

    memcpy(result, first, first_length);
    if (delim_between) result[first_length] = delim_between;
    memcpy(result + first_length + extra, second, second_length);
    if (result_length)
    {
        *result_length = length;
    }
    return result;
}

void _log_message(const char* prefix, const size_t prefix_len,
                  const char* message, const size_t message_len)
{
    char* final =
        concatinate(prefix, prefix_len, message, message_len, '\0', 0, NULL);
    platform_print_string(final);
    platform_print_string("\n");
    free(final);
}

void log_message(const char* message, const size_t message_len)
{
    const char* log = "[LOG]: ";
    const size_t log_len = strlen(log);
    _log_message(log, log_len, message, message_len);
}

void log_error_message(const char* message, const size_t message_len)
{
    const char* error = "[ERROR]: ";
    const size_t error_len = strlen(error);
    _log_message(error, error_len, message, message_len);
}

void log_last_error()
{
    char* last_error_message = platform_get_last_error();
    log_error_message(last_error_message, strlen(last_error_message));
    if (strlen(last_error_message))
    {
        platform_local_free(last_error_message);
    }
}

void log_file_error(const char* file_path)
{
    char* last_error_message = platform_get_last_error();
    size_t error_message_length = 0;
    char* error_message =
        concatinate(file_path, strlen(file_path), last_error_message,
                    strlen(last_error_message), ' ', 0, &error_message_length);

    log_error_message(error_message, error_message_length);

    if (strlen(last_error_message))
    {
        platform_local_free(last_error_message);
    }
    free(error_message);
}
