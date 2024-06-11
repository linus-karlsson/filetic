#pragma once

char* concatinate(const char* first, const size_t first_length, const char* second, const size_t second_length, const char delim_between, size_t* result_length);
void  _log_message(const char* prefix, const size_t prefix_len, const char* message, const size_t message_len);
void  log_message(const char* message, const size_t message_len);
void  log_error_message(const char* message, const size_t message_len);
void  log_last_error();
void  log_file_error(const char* file_path);
