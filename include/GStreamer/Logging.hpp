#ifndef LOGGING_HPP
#define LOGGING_HPP

// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg, cppcoreguidelines-macro-usage, hicpp-vararg)

#ifdef IN_DEBUG

#define MP_PRINT(   msg, ...) g_print(   msg __VA_OPT__(,) __VA_ARGS__)
#define MP_PRINTERR(msg, ...) g_printerr(msg __VA_OPT__(,) __VA_ARGS__)
#define MP_LOGMSG(  msg, ...) g_message( msg __VA_OPT__(,) __VA_ARGS__)
#define MP_LOGWARN( msg, ...) g_warning( msg __VA_OPT__(,) __VA_ARGS__)
#define MP_LOGERROR(msg, ...) g_error(   msg __VA_OPT__(,) __VA_ARGS__)
// #define MP_LOG(  msg, ...) g_log(     msg __VA_OPT__(,) __VA_ARGS__)

#else

#define MP_PRINT(   msg, ...)
#define MP_PRINTERR(msg, ...)
#define MP_LOGMSG(  msg, ...)
#define MP_LOGWARN( msg, ...)
#define MP_LOGERROR(msg, ...)
// #define MP_LOG(     msg, ...)

#endif

// NOLINTEND(cppcoreguidelines-pro-type-vararg, cppcoreguidelines-macro-usage, hicpp-vararg)

#endif // LOGGING_HPP
