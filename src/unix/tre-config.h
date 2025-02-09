/* tre-config.h.in.  This file has all definitions that are needed in
   `tre.h'.  Note that this file must contain only the bare minimum
   of definitions without the TRE_ prefix to avoid conflicts between
   definitions here and definitions included from somewhere else. */

/* Define to 1 if you have the <libutf8.h> header file. */
#undef HAVE_LIBUTF8_H

/* Define to 1 if the system has the type `reg_errcode_t'. */
#undef HAVE_REG_ERRCODE_T

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H

/* Define to 1 if you have the <wchar.h> header file. */
#undef HAVE_WCHAR_H

/* Define if you want to enable approximate matching functionality. */
#define TRE_APPROX

/* Define to enable multibyte character set support. */
#undef TRE_MULTIBYTE

/* Define to the absolute path to the system tre.h */
#undef TRE_SYSTEM_REGEX_H_PATH

/* Define to include the system regex.h from tre.h */
#undef TRE_USE_SYSTEM_REGEX_H

/* Define to enable wide character (wchar_t) support. */
#undef TRE_WCHAR

/* TRE version string. */
#define TRE_VERSION "3.0"

/* TRE version level 1. */
#undef TRE_VERSION_1

/* TRE version level 2. */
#undef TRE_VERSION_2

/* TRE version level 3. */
#define TRE_VERSION_3

#define TRE_REGEX_T_FIELD value
