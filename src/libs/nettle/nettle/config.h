/* Handmade config.h for inclusion in Tremfusion */

#define WITH_PUBLIC_KEY 1
#define HAVE_LIBGMP 1

#if _WIN32
# include <malloc.h>
# define HAVE_ALLOCA 1
#elif __GNUC__
# include <alloca.h>
# define HAVE_ALLOCA 1
#endif

#if __GNUC__
# define NORETURN __attribute__ ((__noreturn__))
# define PRINTF_STYLE(f, a) __attribute__ ((__format__ (__printf__, f, a)))
# define UNUSED __attribute__ ((__unused__))
#else
# define NORETURN
# define PRINTF_STYLE(f, a)
# define UNUSED
#endif

