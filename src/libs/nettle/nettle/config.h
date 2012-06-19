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
