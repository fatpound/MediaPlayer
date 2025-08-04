#ifndef LOGGING_HPP
#define LOGGING_HPP

#define MP_PRINT   (msg, ...) g_print   (msg, __VA_ARGS__);
#define MP_PRINTERR(msg, ...) g_printerr(msg, __VA_ARGS__);
#define MP_LOG     (msg, ...) g_log     (msg, __VA_ARGS__);
#define MP_LOGMSG  (msg, ...) g_message (msg, __VA_ARGS__);
#define MP_LOGWARN (msg, ...) g_warning (msg, __VA_ARGS__);
#define MP_LOGERROR(msg, ...) g_error   (msg, __VA_ARGS__);

#endif // LOGGING_HPP
