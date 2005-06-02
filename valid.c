/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
/*# undef		HAVE_REGEX_H*/
# include	<stdlib.h>
# include	<string.h>
# include	"pager.h"
# include	"valid.h"

valid_t *
v_new (char *pattern)
{
	valid_t	*v;
# if		! HAVE_REGEX_H	
	int	cnt, siz;
	char	*ptr, *sav;
	int	n;
# endif		/* ! HAVE_REGEX_H */

	if (v = (valid_t *) malloc (sizeof (valid_t))) {
# if		HAVE_REGEX_H
		if (regcomp (& v -> r, pattern, REG_EXTENDED)) {
			free (v);
			v = NULL;
		}
# else		/* HAVE_REGEX_H */
		v -> pats = NULL;
		v -> lens = NULL;
		v -> malways = 0;
		if (! strcmp (pattern, "-"))
			v -> malways = 1;
		else if (pattern = strdup (pattern)) {
			cnt = 0;
			siz = 0;
			for (ptr = pattern; ptr; ) {
				sav = ptr;
				if (ptr = strchr (ptr, '|'))
					*ptr++ = '\0';
				if (cnt >= siz) {
					siz += 4;
					if (! (v -> pats = Realloc (v -> pats, (siz + 1) * sizeof (char *))))
						break;
				}
				if (! (v -> pats[cnt] = strdup (sav))) {
					while (--cnt >= 0)
						free (v -> pats[cnt]);
					free (v -> pats);
					v -> pats = NULL;
					break;
				}
				++cnt;
			}
			free (pattern);
			if (v -> pats) {
				v -> pats[cnt] = NULL;
				if (! (v -> lens = (int *) malloc ((cnt + 1) * sizeof (int)))) {
					for (n = 0; n < cnt; ++n)
						free (v -> pats[n]);
					free (v -> pats);
					v -> pats = NULL;
				} else
					for (n = 0; n < cnt; ++n)
						v -> lens[n] = strlen (v -> pats[n]);
			}
			if (! v -> pats) {
				free (v);
				v = NULL;
			}
		}
# endif		/* HAVE_REGEX_H */
	}
	return v;
}

Bool
v_alidate (valid_t *v, char *str, int *start, int *end)
{
	Bool		ret = False;
# if		HAVE_REGEX_H
	regmatch_t	mt;
	
	if (! regexec (& v -> r, str, 1, & mt, 0)) {
		*start = mt.rm_so;
		*end = mt.rm_eo;
		ret = True;
	}
# else		/* HAVE_REGEX_H */
	int	n;
	
	if (v -> pats) {
		for (n = 0; v -> pats[n]; ++n)
			if (! strncmp (v -> pats[n], str, v -> lens[n]))
				break;
		if (v -> pats[n]) {
			*start = 0;
			*end = v -> lens[n];
			ret = True;
		}
	} else if (v -> malways) {
		*start = 0;
		*end = 0;
		ret = True;
	}
# endif		/* HAVE_REGEX_H */
	return ret;
}

void
v_free (valid_t *v)
{
# if		HAVE_REGEX_H
	regfree (& v -> r);
# else		/* HAVE_REGEX_H */
	int	n;
	
	if (v -> pats) {
		for (n = 0; v -> pats[n]; ++n)
			free (v -> pats[n]);
		free (v -> pats);
	}
	if (v -> lens) {
		free (v -> lens);
		v -> lens = NULL;
	}
# endif		/* HAVE_REGEX_H */
	free (v);
}
