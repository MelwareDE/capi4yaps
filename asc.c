/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# include	<stdio.h>
# include	<stdlib.h>
# include	<ctype.h>
# include	<string.h>
# include	"pager.h"

/*{{{	typedefs */
typedef struct {
# ifndef	NDEBUG
# define	MAGIC		MKMAGIC('a', 's', 'c', '\0')
	long	magic;
# endif		/* NDEBUG */
	void	*sp;
	void	*ctab;
	void	(*logger) (char, char *, ...);
	char	*callid;
	int	deftout;
	char	*alogin;
	char	*alogout;
	char	*apid;
	char	*amsg;
	char	*anext;
	char	*async;
	
	date_t	delay;
	date_t	expire;
	Bool	rds;
}	asc;
/*}}}*/
/*{{{	convert */
static char *
escape_string (asc *a, char *str)
{
	int	c;
	char	ch, prev;
	char	*ret;
	int	n;
	
	if (ret = malloc (strlen (str) * 2 + 16)) {
		prev = '\0';
		ch = '\0';
		for (n = 0; *str; prev = ch) {
			c = cv_conv (a -> ctab, (char_t) *str++);
			if (c < 0)
				continue;
			ch = (char) c;
			if ((ch == '<') && isspace (prev)) {
				ret[n++] = '\\';
				ret[n++] = '<';
			} else if (ch == ' ') {
				ret[n++] = '\\';
				ret[n++] = 's';
			} else if (ch == '\a') {
				ret[n++] = '\\';
				ret[n++] = 'a';
			} else if (ch == '\b') {
				ret[n++] = '\\';
				ret[n++] = 'b';
			} else if (ch == '\f') {
				ret[n++] = '\\';
				ret[n++] = 'f';
			} else if (ch == '\n') {
				ret[n++] = '\\';
				ret[n++] = 'n';
			} else if (ch == '\r') {
				ret[n++] = '\\';
				ret[n++] = 'r';
			} else if (ch == '\t') {
				ret[n++] = '\\';
				ret[n++] = 't';
			} else if (ch) {
				if ((ch == '\\') || (ch == '^') || (ch == '%'))
					ret[n++] = '\\';
				ret[n++] = ch;
			}
		}
		ret[n] = '\0';
	}
	return ret;
}
			
static char *
convert_asc (asc *a, char *pat, char *pid, char *msg)
{
	char	*str;
	int	siz, len;
	char	*ptr;
	int	plen;
	char	scr[32];

	str = NULL;
	siz = 0;
	len = 0;
	while (*pat) {
		if (len + 2 >= siz) {
			siz += 32;
			if (! (str = Realloc (str, siz + 2)))
				break;
		}
		if (*pat == '\\') {
			++pat;
			if (*pat) {
				ptr = NULL;
				switch (*pat) {
				case 'C':	ptr = a -> callid;	break;
				case 'P':	ptr = pid;		break;
				case 'M':	ptr = msg;		break;
				case 'R':
					strcpy (scr, (a -> rds ? "1" : "0"));
					ptr = scr;
					break;
				default:
					str[len++] = '\\';
					str[len++] = *pat;
					break;
				}
				if (ptr && (ptr = escape_string (a, ptr))) {
					if (plen = strlen (ptr)) {
						if (len + plen + 2 >= siz) {
							siz = len + plen + 32;
							if (! (str = Realloc (str, siz)))
								break;
						}
						strcpy (str + len, ptr);
						len += plen;
					}
					free (ptr);
				}
				++pat;
			}
		} else if (! iscntrl (*pat))
			str[len++] = *pat++;
		else {
			str[len++] = ' ';
			++pat;
		}
	}
	if (str)
		str[len] = '\0';
	return str;
}
/*}}}*/
/*{{{	general sending routine */
static int
do_send (asc *a, char *what, char *pat, char *pid, char *msg)
{
	int	ret;
	char	*str;
	
	ret = ERR_FAIL;
	if (a && a -> sp)
		if (pat && *pat) {
			if (str = convert_asc (a, pat, pid, msg)) {
				if (tty_send_expect (a -> sp, a -> deftout, str, NULL) != -1) {
					V (1, ("Ascii %s sent\n", what));
					ret = NO_ERR;
				} else
					V (1, ("Unable to send %s\n", what));
				free (str);
			}
		} else
			ret = NO_ERR;
	return ret;
}
/*}}}*/
/*{{{	login/logout/transmit/next/sync */
int
asc_login (void *ap, string_t *cid)
{
	asc	*a = (asc *) ap;

	MCHK (a);
	if (a) {
		if (a -> callid)
			free (a -> callid);
		a -> callid = sextract (cid);
	}
	return a ? do_send (a, "login", a -> alogin, NULL, NULL) : ERR_ABORT;
}

int
asc_logout (void *ap)
{
	asc	*a = (asc *) ap;

	MCHK (a);
	return a ? do_send (a, "logout", a -> alogout, NULL, NULL) : ERR_FATAL;
}

int
asc_transmit (void *ap, char *pid, char *msg)
{
	asc	*a = (asc *) ap;
	int	n;
	
	MCHK (a);
	if (! a)
		return ERR_FATAL;
	if ((n = do_send (a, "pagerid", a -> apid, pid, NULL)) != NO_ERR)
		return n;
	return do_send (a, "message", a -> amsg, NULL, msg);
}

int
asc_next (void *ap)
{
	asc	*a = (asc *) ap;

	MCHK (a);
	return a ? do_send (a, "next", a -> anext, NULL, NULL) : ERR_FATAL;
}

int
asc_sync (void *ap)
{
	asc	*a = (asc *) ap;
	
	MCHK (a);
	return a ? do_send (a, "sync", a -> async, NULL, NULL) : ERR_FATAL;
}
/*}}}*/
/*{{{	config */
void
asc_config (void *ap, void (*logger) (char, char *, ...),
	    int deftout, char *alogin, char *alogout, char *apid, char *amsg, char *anext, char *async,
	    date_t *delay, date_t *expire, Bool rds)
{
	asc	*a = (asc *) ap;
	
	MCHK (a);
	if (a) {
		a -> logger = logger;
		if (deftout != -1)
			a -> deftout = deftout;
		if (alogin) {
			if (a -> alogin)
				free (a -> alogin);
			a -> alogin = strdup (alogin);
		}
		if (alogout) {
			if (a -> alogout)
				free (a -> alogout);
			a -> alogout = strdup (alogout);
		}
		if (apid) {
			if (a -> apid)
				free (a -> apid);
			a -> apid = strdup (apid);
		}
		if (amsg) {
			if (a -> amsg)
				free (a -> amsg);
			a -> amsg = strdup (amsg);
		}
		if (anext) {
			if (a -> anext)
				free (a -> anext);
			a -> anext = strdup (anext);
		}
		if (async) {
			if (a -> async)
				free (a -> async);
			a -> async = strdup (async);
		}
		if (delay)
			a -> delay = *delay;
		else
			dat_clear (& a -> delay);
		if (expire)
			a -> expire = *expire;
		else
			dat_clear (& a -> expire);
		a -> rds = rds;
	}
}

void
asc_set_convtable (void *ap, void *ctab)
{
	asc	*a = (asc *) ap;
	
	MCHK (a);
	if (a) {
		if (a -> ctab)
			cv_free (a -> ctab);
		a -> ctab = ctab;
	}
}

void
asc_add_convtable (void *ap, void *ctab)
{
	asc	*a = (asc *) ap;

	MCHK (a);
	if (a) {
		if (! a -> ctab)
			a -> ctab = cv_new ();
		if (a -> ctab)
			cv_merge (a -> ctab, ctab, True);
	}
}
/*}}}*/
/*{{{	new/free/etc */
void *
asc_new (void *sp)
{
	asc	*a;
	
	if (a = (asc *) malloc (sizeof (asc))) {
# ifndef	NDEBUG
		a -> magic = MAGIC;
# endif		/* NDEBUG */
		a -> sp = sp;
		a -> ctab = NULL;
		a -> logger = NULL;
		a -> callid = NULL;
		a -> deftout = 10;
		a -> alogin = NULL;
		a -> alogout = NULL;
		a -> apid = NULL;
		a -> amsg = NULL;
		a -> anext = NULL;
		a -> async = NULL;
		dat_clear (& a -> delay);
		dat_clear (& a -> expire);
		a -> rds = False;
	}
	return (void *) a;
}

void *
asc_free (void *ap)
{
	asc	*a = (asc *) ap;
	
	MCHK (a);
	if (a) {
		if (a -> ctab)
			cv_free (a -> ctab);
		if (a -> callid)
			free (a -> callid);
		if (a -> alogin)
			free (a -> alogin);
		if (a -> alogout)
			free (a -> alogout);
		if (a -> apid)
			free (a -> apid);
		if (a -> amsg)
			free (a -> amsg);
		if (a -> anext)
			free (a -> anext);
		if (a -> async)
			free (a -> async);
		free (a);
	}
	return NULL;
}

int
asc_preinit (void)
{
	return 0;
}

void
asc_postdeinit (void)
{
}
/*}}}*/
