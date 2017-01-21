
#ifndef BALLER_I18N_H
#define BALLER_I18N_H

#include <config.h>

#if HAVE_LIBINTL_H

#include <libintl.h>
#define _(string) ((string)[0]?gettext(string):(string))
#define gettext_noop(string) string
#define N_(string) gettext_noop(string)

#else

#define _(string) (string)
#define N_(string) string

#endif  /* HAVE_GETTEXT */

#endif  /* BALLER_I18N_H */
