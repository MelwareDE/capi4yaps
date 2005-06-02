/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# include	<stdio.h>
# include	<stdlib.h>
# include	<stdarg.h>
# include	<ctype.h>
# include	<string.h>
# include	<time.h>
# include	"pager.h"

/*{{{	utility functions */
char *
skip (char *str)
{
	while (*str && (! isspace (*str)))
		++str;
	if (*str) {
		*str++ = '\0';
		while (isspace (*str))
			++str;
	}
	return str;
}

char *
skipch (char *str, char ch)
{
	while (*str && (*str != ch))
		++str;
	if (*str) {
		*str++ = '\0';
		while (isspace (*str))
			++str;
	}
	return str;
}

char *
getline (FILE *fp, Bool cont)
{
	char	*buf;
	int	size;
	char	*ret;
	int	len;
	char	*ptr;

	size = 256;
	if (! (buf = malloc (size + 2)))
		return NULL;
	len = 0;
	while (ret = fgets (buf + len, size - len - 2, fp)) {
		if (ptr = strchr (buf + len, '\n')) {
			*ptr = '\0';
			len += strlen (buf + len);
			if (! cont)
				break;
			if (len && (buf[len - 1] == '\\')) {
				--len;
				buf[len] = '\0';
			} else
				break;
		} else
			len += strlen (buf + len);
		if (len + 64 >= size) {
			size += size;
			if (! (buf = Realloc (buf, size + 2)))
				break;
		}
	}
	if ((! ret) && buf) {
		free (buf);
		buf = NULL;
	}
	return len || ret ? buf : NULL;
}

int
verbose_out (char *fmt, ...)
{
	va_list	par;
	
	va_start (par, fmt);
	vfprintf (stdout, fmt, par);
	va_end (par);
	return 0;
}
/*}}}*/
/*{{{	string_t handling */
string_t *
snewc (char *str)
{
	string_t	*s;
	
	if (s = (string_t *) malloc (sizeof (string_t))) {
		s -> str = NULL;
		if (str) {
			s -> len = strlen (str);
			s -> size = s -> len + 1;
			if (s -> str = (char_t *) malloc (sizeof (char_t) * s -> size))
				memcpy (s -> str, str, s -> len);
			else {
				free (s);
				s = NULL;
			}
		} else {
			s -> len = 0;
			s -> size = 0;
		}
	}
	return s;
}

string_t *
snew (char_t *str, int len)
{
	string_t	*s;
	
	if (s = (string_t *) malloc (sizeof (string_t))) {
		s -> str = NULL;
		s -> len = 0;
		s -> size = 0;
		if (len > 0)
			if (s -> str = (char_t *) malloc (sizeof (char_t) * (len + 1))) {
				s -> size = len + 1;
				if (str) {
					s -> len = len;
					memcpy (s -> str, str, len);
				} else
					s -> len = 0;
			} else {
				free (s);
				s = NULL;
			}
	}
	return s;
}

Bool
sexpand (string_t *s, int nsize)
{
	if (s && (nsize + 2 > s -> size)) {
		s -> size = nsize + 2;
		if (! (s -> str = (char_t *) Realloc (s -> str, sizeof (char_t) * s -> size))) {
			s -> size = 0;
			s -> len = 0;
			return False;
		}
	}
	return True;
}

Bool
scopy (string_t *dst, string_t *src)
{
	if (dst && src && sexpand (dst, src -> len + 1)) {
		if (src -> str) {
			memcpy (dst -> str, src -> str, src -> len);
			dst -> len = src -> len;
		} else
			dst -> len = 0;
		return True;
	}
	return False;
}

Bool
scat (string_t *dst, string_t *src)
{
	if (dst && src && sexpand (dst, dst -> len + src -> len + 1)) {
		if (src -> str) {
			memcpy (dst -> str + dst -> len, src -> str, src -> len);
			dst -> len += src -> len;
		}
		return True;
	}
	return False;
}

static Bool
dostr (string_t *dst, char *src, Bool (*func) (string_t *, string_t *))
{
	Bool		ret;
	string_t	*rsrc;
			
	ret = False;
	if (dst)
		if (src) {
			if (rsrc = snewc (src)) {
				ret = (*func) (dst, rsrc);
				sfree (rsrc);
			}
		} else
			ret = True;
	return ret;
}

Bool
scopyc (string_t *dst, char *src)
{
	return dostr (dst, src, scopy);
}

Bool
scatc (string_t *dst, char *src)
{
	return dostr (dst, src, scat);
}

string_t *
scut (string_t *str, int start, int len)
{
	string_t	*res;

	if (len < 0)
		len = str ? str -> len - start : 0;
	if (res = snew (NULL, len + 1)) {
		if (str -> len > start) {
			if (str -> len - start < len)
				len = str -> len - start;
		} else
			len = 0;
		if (len > 0)
			memcpy (res -> str, str -> str + start, len);
		res -> len = len;
	}
	return res;
}

void
sdel (string_t *str, int start, int len)
{
	int	size;
	
	if (str -> len > start) {
		if (str -> len - start < len)
			len = str -> len - start;
	} else
		len = 0;
	if (len > 0) {
		size = str -> len - (start + len);
		if (size > 0)
			memcpy (str -> str + start, str -> str + start + len, str -> len - (start + len));
		str -> len -= len;
	}
}

Bool
sput (string_t *str, string_t *ins, int pos, int len)
{
	if ((len < 0) || (len > ins -> len))
		len = ins -> len;
	if (len + pos >= str -> size)
		if (! sexpand (str, len + pos + 1))
			return False;
	memcpy (str -> str + pos, ins -> str, len);
	if (str -> len < len + pos)
		str -> len = len + pos;
	return True;
}
	
Bool
sputc (string_t *str, char *ins, int pos, int len)
{
	Bool		ret;
	string_t	*rins;
			
	ret = False;
	if (str && ins && (rins = snewc (ins))) {
		ret = sput (str, rins, pos, len);
		sfree (rins);
	}
	return ret;
}

char *
sextract (string_t *s)
{
	char	*ret;
	
	ret = NULL;
	if (s)
		if (ret = malloc (s -> len + 1))
			if (s -> str) {
				memcpy (ret, s -> str, s -> len);
				ret[s -> len] = '\0';
			} else
				ret[0] = '\0';
	return ret;
}

char *
schar (string_t *s)
{
	if (s) {
		if (s -> len + 1>= s -> size)
			sexpand (s, s -> len + 2);
		if (s -> len + 1 < s -> size) {
			s -> str[s -> len] = '\0';
			return (char *) s -> str;
		}
	}
	return NULL;
}

void *
sfree (string_t *s)
{
	if (s) {
		if (s -> str)
			free (s -> str);
		free (s);
	}
	return NULL;
}

void
srelease (string_t *s)
{
	if (s -> size + 1 > s -> len) {
		s -> size = s -> len + 1;
		if (! (s -> str = (char_t *) Realloc (s -> str, sizeof (char_t) * (s -> size + 2)))) {
			s -> size = 0;
			s -> len = 0;
		}
	}
}

Bool
siscntrl (string_t *s, int pos)
{
	return (s && (pos >= 0) && (pos < s -> len) && iscntrl (s -> str[pos])) ? True : False;
}

Bool
sisspace (string_t *s, int pos)
{
	return (s && (pos >= 0) && (pos < s -> len) && isspace (s -> str[pos])) ? True : False;
}

Bool
sisdigit (string_t *s, int pos)
{
	return (s && (pos >= 0) && (pos < s -> len) && isdigit (s -> str[pos])) ? True : False;
}

int
stoi (string_t *s)
{
	int	ret;
	
	ret = 0;
	if (s && s -> str) {
		s -> str[s -> len] = '\0';
		ret = atoi ((char *) s -> str);
	}
	return ret;
}
/*}}}*/
/*{{{	date_t handling */
# if		! HAVE_MEMSET && ! HAVE_BZERO
static void
dozero (void *p, int len)
{
	unsigned char	*ptr = (char *) p;
	
	while (len-- > 0)
		*ptr++ = 0;
}
# endif		/* ! HAVE_MEMSET && ! HAVE_BZERO */

date_t *
dat_free (date_t *d)
{
	if (d)
		free (d);
	return NULL;
}

date_t *
dat_parse (char *str)
{
	date_t		*d;
	time_t		tim;
	struct tm	*tt;
	struct tm	tm;
	Bool		add;
	char		*ptr, *sav;
	char		*p1, *p2;
	int		mode;
	int		n, val;
	char		sep;
	date_t		tmp;

	d = NULL;
	if ((! str) || (str = strdup (str))) {
		if (d = (date_t *) malloc (sizeof (date_t))) {
			time (& tim);
			if (tt = localtime (& tim)) {
				d -> day = tt -> tm_mday;
				d -> mon = tt -> tm_mon + 1;
				d -> year = tt -> tm_year + 1900;
				d -> hour = tt -> tm_hour;
				d -> min = tt -> tm_min;
				d -> sec = tt -> tm_sec;
			} else {
				d -> day = 1;
				d -> mon = 1;
				d -> year = 1970;
				d -> hour = 0;
				d -> min = 0;
				d -> sec = 0;
			}
			if (str && strcmp (str, "now")) {
				if (*str == '+') {
					add = True;
					ptr = str + 1;
				} else {
					add = False;
					ptr = str;
				}
				tmp.day = -1;
				tmp.mon = -1;
				tmp.year = -1;
				tmp.hour = -1;
				tmp.min = -1;
				tmp.sec = -1;
				while (*ptr) {
					sav = ptr;
					ptr = skip (ptr);
					if (strchr (sav, '.')) {
						sep = '.';
						mode = 1;
					} else if (strchr (sav, '/')) {
						sep = '/';
						mode = 2;
					} else if (strchr (sav, ':')) {
						sep = ':';
						mode = 3;
					} else {
						sep = '\0';
						mode = 0;
					}
					for (p1 = sav, n = 0; *p1; ++n) {
						p2 = p1;
						p1 = skipch (p1, sep);
						val = atoi (p2);
						switch (mode) {
						case 0:
							tmp.sec = 0;
							if (val < 30) {
								tmp.hour = val;
								tmp.min = 0;
							} else {
								tmp.hour = val / 60;
								tmp.min = val % 60;
							}
							break;
						case 1:
							if (n == 0)
								tmp.day = val;
							else if (n == 1)
								tmp.mon = val;
							else if (n == 2) {
								if ((! add) && (val < 1900))
									val += 1900;
								tmp.year = val;
							}
							break;
						case 2:
							if (n == 0)
								tmp.mon = val;
							else if (n == 1)
								tmp.day = val;
							else if (n == 2) {
								if ((! add) && (val < 1900))
									val += 1900;
								tmp.year = val;
							}
							break;
						case 3:
							if (n == 0) {
								tmp.hour = val;
								tmp.min = 0;
								tmp.sec = 0;
							} else if (n == 1)
								tmp.min = val;
							else if (n == 2)
								tmp.sec = val;
							break;
						}
					}
				}
				if (add) {
					if (tmp.day != -1)
						d -> day += tmp.day;
					if (tmp.mon != -1)
						d -> mon += tmp.mon;
					if (tmp.year != -1)
						d -> year += tmp.year;
					if (tmp.hour != -1)
						d -> hour += tmp.hour;
					if (tmp.min != -1)
						d -> min += tmp.min;
					if (tmp.sec != -1)
						d -> sec += tmp.sec;
				} else {
					if (tmp.day != -1)
						d -> day = tmp.day;
					if (tmp.mon != -1)
						d -> mon = tmp.mon;
					if (tmp.year != -1)
						d -> year = tmp.year;
					if (tmp.hour != -1)
						d -> hour = tmp.hour;
					if (tmp.min != -1)
						d -> min = tmp.min;
					if (tmp.sec != -1)
						d -> sec = tmp.sec;
				}
# if		HAVE_MEMSET
				memset (& tm, 0, sizeof (tm));
# elif		HAVE_BZERO				
				bzero (& tm, sizeof (tm));
# else
				dozero (& tm, sizeof (tm));
# endif		/* ! HAVE_MEMSET && ! HAVE_BZERO */
				tm.tm_mday = d -> day;
				tm.tm_mon = d -> mon - 1;
				tm.tm_year = d -> year - 1900;
				tm.tm_hour = d -> hour;
				tm.tm_min = d -> min;
				tm.tm_sec = d -> sec;
				tm.tm_isdst = tt -> tm_isdst;
				if (mktime (& tm) == (time_t) -1) 
					d = dat_free (d);
				else {
					d -> day = tm.tm_mday;
					d -> mon = tm.tm_mon + 1;
					d -> year = tm.tm_year + 1900;
					d -> hour = tm.tm_hour;
					d -> min = tm.tm_min;
					d -> sec = tm.tm_sec;
				}
			}
		}
		if (str)
			free (str);
	}
	return d;
}

int
dat_diff (date_t *d1, date_t *d2)
{
	int	v1, v2;
	
	if (d1)
		v1 = (((((d1 -> year * 12 + d1 -> mon) * 31 + d1 -> day) * 24 + d1 -> hour) * 60 + d1 -> min) * 60 + d1 -> sec);
	else
		v1 = 0;
	if (d2)
		v2 = (((((d2 -> year * 12 + d2 -> mon) * 31 + d2 -> day) * 24 + d2 -> hour) * 60 + d2 -> min) * 60 + d2 -> sec);
	else
		v2 = 0;
	return v2 - v1;
}

void
dat_clear (date_t *d)
{
	d -> day = 0;
	d -> mon = 0;
	d -> year = 0;
	d -> hour = 0;
	d -> min = 0;
	d -> sec = 0;
}

void
dat_localtime (date_t *d)
{
	time_t		tim;
	struct tm	*tt;
	
	time (& tim);
	if (tt = localtime (& tim)) {
		d -> day = tt -> tm_mday;
		d -> mon = tt -> tm_mon + 1;
		d -> year = tt -> tm_year + 1900;
		d -> hour = tt -> tm_hour;
		d -> min = tt -> tm_min;
		d -> sec = tt -> tm_sec;
	} else
		dat_clear (d);
}
/*}}}*/
