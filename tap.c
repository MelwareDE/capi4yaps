/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# include	<stdio.h>
# include	<stdlib.h>
# include	<ctype.h>
# include	<string.h>
# include	"pager.h"

/*{{{	definitions & typedefs */
# define	DEF_SERVICE_TYPE	"PG"
# define	DEF_TERMINAL_TYPE	'1'

# define	CHKTYPE(sss,ccc)	((((sss)[0] & 0xff) << 16) | (((sss)[1] & 0xff) << 8) | ((ccc) & 0xff))
# define	CHKSTYPE(cc1,cc2,cc3)	((((cc1) & 0xff) << 16) | (((cc2) & 0xff) << 8) | ((cc3) & 0xff))

typedef enum {
	None,
	PG1
}	Type;

typedef struct _rmsg {
	string_t	*str;
	int		rnr;
	struct _rmsg	*next;
}	rmsg;

typedef struct {
# ifndef	NDEBUG
# define	MAGIC		MKMAGIC ('t', 'a', 'p', '\0')
	long	magic;
# endif		/* NDEBUG */
	void	*sp;		/* the serial connection		*/
	void	*ctab;		/* the conversion table			*/
	void	(*logger) (char, char *, ...);
	string_t
		*callid;	/* caller id				*/
	Type	typ;		/* the connection type			*/
	Bool	pre16;		/* if p.version before 1.6 is used	*/
	/* timing and retry values					*/
	int	t1, t2, t3, t4, t5;
	int	n1, n2, n3;
	int	licnt, locnt;
	
	rmsg	*r, *prv;	/* response message(s) on callback	*/
}	tap;
/*}}}*/
/*{{{	convert */
static int
field_convert (tap *t, string_t *ptr, string_t *cf)
{
	int	n;
	int	ch;

	cf -> str = NULL;
	cf -> size = ptr -> len + 32;
	if (! (cf -> str = malloc (cf -> size + 2)))
		return ERR_FATAL;
	for (n = 0, cf -> len = 0; n < ptr -> len; ++n) {
		if (cf -> len + 2 >= cf -> size)
			if (! sexpand (cf, cf -> size + 32))
				return ERR_FATAL;
		ch = cv_conv (t -> ctab, ptr -> str[n]);
		if (ch != -1)
			switch ((char_t) ch) {
			case '\x0d': case '\x0a': case '\x1b':
			case '\x02': case '\x03': case '\x1f':
			case '\x17': case '\x04': case '\x1a':
				if (! t -> pre16) {
					cf -> str[cf -> len++] = '\x1a';
					cf -> str[cf -> len++] = (char_t) (ch + 0x40);
				}
				break;
			default:
				cf -> str[cf -> len++] = (char_t) ch;
				break;
		}
	}
	if (cf -> str) {
		cf -> str[cf -> len] = '\0';
		return NO_ERR;
	}
	return ERR_FATAL;
}

static char	chkstr[] = "0123456789:;<=>?";
static string_t **
convert_tap (tap *t, string_t **field)
{
	string_t	**ret;
	int		fcnt;
	string_t	*cf;
	char_t		*ptr;
	int		cnt, siz;
	char_t		tmp[260];
	Bool		needchk;
	unsigned long	chk;
	int		fld, off, len;
	int		n, m;

	ret = NULL;
	for (fcnt = 0; field[fcnt]; ++fcnt)
		;
	if (cf = (string_t *) malloc ((fcnt + 1) * sizeof (string_t))) {
		for (fld = 0; fld < fcnt; ++fld)
			if (field_convert (t, field[fld], & cf[fld]) != NO_ERR) {
				while (--fld >= 0)
					free (cf[fld].str);
				free (cf);
				break;
			}
		if (fld < fcnt)
			return NULL;
		cnt = 0;
		siz = 0;
		n = 0;
		needchk = False;
		for (fld = 0, off = 0; fld < fcnt; ) {
			ptr = cf[fld].str + off;
			len = cf[fld].len - off;
			do {
				if (! n)
					tmp[n++] = '\x02';
				if (len) {
					tmp[n++] = *ptr++;
					--len;
					++off;
				}
				if (! len) {
					off = 0;
					++fld;
					tmp[n++] = '\r';
					if (fld == fcnt) {
						tmp[n++] = '\x03';
						needchk = True;
					} else if (n >= 230) {
						tmp[n++] = '\x17';
						needchk = True;
					}
				} else if (n >= 250) {
					tmp[n++] = '\x1f';
					needchk = True;
				}
				if (needchk) {
					needchk = False;
					for (m = 0, chk = 0; m < n; ++m)
						chk += tmp[m] & 0xff;
					tmp[n++] = chkstr[(chk >> 8) & 0xf];
					tmp[n++] = chkstr[(chk >> 4) & 0xf];
					tmp[n++] = chkstr[chk & 0xf];
					tmp[n++] = '\r';
					tmp[n] = '\0';
					if (cnt >= siz) {
						siz += 4;
						if (! (ret = (string_t **) Realloc (ret, (siz + 1) * sizeof (string_t *))))
							return NULL;
					}
					if (! (ret[cnt] = snew (tmp, n))) {
						while (--cnt >= 0)
							sfree (ret[cnt]);
						free (ret);
						return NULL;
					}
					++cnt;
					ret[cnt] = NULL;
					n = 0;
				}
			}	while (len);
		}
		for (fld = 0; fld < fcnt; ++fld)
			free (cf[fld].str);
		free (cf);
	}
	return ret;
}
/*}}}*/
/*{{{	callback interface for receiving response numbers */
static void
getresponse (void *sp, string_t *s, char_t ch, void *data)
{
	tap	*t = (tap *) data;
	rmsg	*r;
	
	if (t && (! t -> pre16) && s && (s -> len > 0))
		if (sisdigit (s, 0)) {
			if (r = (rmsg *) malloc (sizeof (rmsg)))
				if (r -> str = snew (s -> str, s -> len + 2)) {
					r -> rnr = stoi (r -> str);
					r -> next = NULL;
					if (t -> prv)
						t -> prv -> next = r;
					else
						t -> r = r;
					t -> prv = r;
				} else
					free (r);
		} else if ((! siscntrl (s, 0)) && (r = t -> prv) && scatc (r -> str, "\n")) {
			if (sisspace (s, 0))
				sdel (s, 0, 1);
			scat (r -> str, s);
		}
}

static void
free_resp (tap *t)
{
	rmsg	*tmp;
	
	if (t) {
		while (t -> r) {
			tmp = t -> r;
			t -> r = t -> r -> next;
			sfree (tmp -> str);
			free (tmp);
		}
		t -> prv = NULL;
	}
}

static void
setcb (tap *t)
{
	if (! t -> pre16) {
		free_resp (t);
		tty_suspend_callback (t -> sp, False);
	}
}

static void
clrcb (tap *t)
{
	if (! t -> pre16) {
		tty_suspend_callback (t -> sp, True);
		free_resp (t);
	}
}
/*}}}*/
/*{{{	response handling */
static int
show_response (tap *t)
{
	rmsg		*r;
	int		nr, typ;
	char		*line, *ptr;
	
	for (r = t -> r, nr = -1; r; r = r -> next) {
		nr = r -> rnr;
		if (line = sextract (r -> str)) {
			for (ptr = line; isdigit (*ptr); ++ptr)
				;
			while (isspace (*ptr))
				++ptr;
		} else
			ptr = "(none)";
		switch (nr) {
		case 110:
			V (1, ("TAP Version: %s\n", ptr));
			break;
		case 111:
			V (1, ("processing input\n"));
			break;
		case 112:
			V (1, ("maximum pages entered\n"));
			break;
		case 113:
			V (1, ("maximum time reached\n"));
			break;
		case 114:
			V (1, ("Welcome: %s\n", ptr));
			break;
		case 115:
			V (1, ("Byebye: %s\n", ptr));
			break;
		case 211:
			V (1, ("Page(s) sent successful\n"));
			break;
		case 212:
			V (1, ("Long message truncated and sent\n"));
			break;
		case 213:
			V (1, ("Message accepted - held for deferred delivery\n"));
			break;
		case 214:
			V (1, ("%d character maximum, message has been truncated and sent\n", atoi (ptr)));
			break;
		case 501:
			V (1, ("Timeout during input\n"));
			break;
		case 502:
			V (1, ("Unexpected character received before transaction\n"));
			break;
		case 503:
			V (1, ("Excessive attempts to send/re-send a transaction with checksum errors\n"));
			break;
		case 504:
			V (1, ("Message have to be empty for tone-only pager\n"));
			break;
		case 505:
			V (1, ("Message must not contain characters for a numeric pager\n"));
			break;
		case 506:
			V (1, ("Excessive invalid pages received\n"));
			break;
		case 507:
			V (1, ("Invalid logon attempt: Incorrectly formed logon sequence\n"));
			break;
		case 508:
			V (1, ("Invalid Logon attempt: Service type and category given is not supported\n"));
			break;
		case 509:
			V (1, ("Invalid Logon attempt: Invalid password supplied\n"));
			break;
		case 510:
			V (1, ("Illegal Pager ID\n"));
			break;
		case 511:
			V (1, ("Invalid Pager ID\n"));
			break;
		case 512:
			V (1, ("Temporarily cannot deliver to Pager ID\n"));
			break;
		case 513:
			V (1, ("Long message rejected for exceeding maximum character length\n"));
			break;
		case 514:
			V (1, ("Checksum error\n"));
			break;
		case 515:
			V (1, ("Message format error\n"));
			break;
		case 516:
			V (1, ("Message quota temporarily exceeded\n"));
			break;
		case 517:
			V (1, ("%d character maximum, message rejected\n", atoi (ptr)));
			break;
		default:
			typ = nr / 100;
			switch (typ) {
			default:
				V (1, ("Response"));
				break;
			case 1:
				V (1, ("Informal"));
				break;
			case 2:
				V (1, ("Success"));
				break;
			case 5:
				V (1, ("Fail"));
				break;
			}
			V (1, (" %03d: %s\n", r -> rnr, ptr));
			break;
		}
		if (line)
			free (line);
	}
	return nr;
}
/*}}}*/
/*{{{	login/logout */
int
tap_login (void *tp, char *stype, char ttype, char *passwd, string_t *callid)
{
	tap	*t = (tap *) tp;
	int	n, ep;
	char	*cbuf;
	int	clen;
	int	err;

	MCHK (t);
	if ((! t) || (! t -> sp))
		return ERR_ABORT;
	if (! stype)
		stype = DEF_SERVICE_TYPE;
	else if (strlen (stype) != 2) {
		V (1, ("Invalid service type %s\n", stype));
		return ERR_ABORT;
	}
	if (! ttype)
		ttype = DEF_TERMINAL_TYPE;
	switch (CHKTYPE (stype, ttype)) {
	case CHKSTYPE ('P', 'G', '1'):
		t -> typ = PG1;
		break;
	}
	if (t -> typ == None) {
		V (1, ("Invalid service/terminal type combination\n"));
		return ERR_ABORT;
	}
	t -> callid = sfree (t -> callid);
	if (callid && (! (t -> callid = snew (callid -> str, callid -> len))))
		return ERR_ABORT;
	for (n = 0; n < t -> n1; ++n)
		if (tty_send (t -> sp, "\r", 1) != 1)
			V (1, ("Unable to send initial CR\n"));
		else if (tty_expect (t -> sp, t -> t1, "ID=", 3, NULL) == 1) {
			V (1, ("Send initial CR\n"));
			break;
		}
	if (n == t -> n1) {
		V (1, ("Didn't got initial response\n"));
		return ERR_ABORT;
	}
	if (cbuf = malloc ((passwd ? strlen (passwd) : 0) + strlen (stype) + 16)) {
		if (passwd)
			sprintf (cbuf, "\x1b%s%c%s\r", stype, ttype, passwd);
		else
			sprintf (cbuf, "\x1b%s%c\r", stype, ttype);
		clen = strlen (cbuf);
	} else {
		V (1, ("Out of memory!\n"));
		return ERR_ABORT;
	}
	err = NO_ERR;
	for (n = 0; n < t -> licnt; ++n)
		if (tty_send (t -> sp, cbuf, clen) != clen)
			V (1, ("Unable to send Logon request\n"));
		else {
			setcb (t);
			ep = tty_expect (t -> sp, t -> t3, "\x06\r", 2, "ID=", 3, "\x15\r", 2, "\x1b\x04\r", 3, NULL);
			if ((! t -> pre16) && t -> r)
				show_response (t);
			if (ep == 1) {
				V (1, ("Login request accepted\n"));
				break;
			} else if (ep == 2)
				V (1, ("Oops, got ID= again\n"));
			else if (ep == 3) 
				V (1, ("Login request not accepted\n"));
			else if (ep == 4) {
				V (1, ("Logout forced\n"));
				n = t -> licnt;
				err = ERR_ABORT;
				break;
			}
		}
	free (cbuf);
	clrcb (t);
	if (n == t -> licnt) {
		V (1, ("Could not login\n"));
		return (err == NO_ERR) ? ERR_FATAL : err;
	}
	if (tty_expect (t -> sp, t -> t3, "\x1b[p\r", 4, NULL) != 1) {
		V (1, ("Didn't got Go Ahead message\n"));
		return ERR_FATAL;
	} else
		V (1, ("Successful login\n"));
	return NO_ERR;
}

int
tap_logout (void *tp)
{
	tap	*t = (tap *) tp;
	int	err;
	int	n, ep;

	MCHK (t);
	if ((! t) || (! t -> sp))
		return ERR_FATAL;
	err = NO_ERR;
	for (n = 0; n < t -> locnt; ++n) 
		if (tty_send (t -> sp, "\x04\r", 2) != 2)
			V (1, ("Unable to send logout request\n"));
		else {
			setcb (t);
			ep = tty_expect (t -> sp, t -> t3, "\x1b\x04\r", 3, "\x1e\r", 2, NULL);
			if ((! t -> pre16) && t -> r)
				show_response (t);
			if (ep == 1) {
				V (1, ("Message sending completed\n"));
				break;
			} else if (ep == 2) {
				V (1, ("Message sending failed (%d)\n", ep));
				err = ERR_FATAL;
				n = t -> locnt;
				break;
			}
		}
	clrcb (t);
	if (n == t -> locnt) {
		if (err == NO_ERR)
			err = ERR_FAIL;
		V (1, ("Unable to logout\n"));
		return err;
	} else
		V (1, ("Logout successful\n"));
	return NO_ERR;
}
/*}}}*/
/*{{{	transmit */
int
tap_transmit (void *tp, string_t **field, Bool last)
{
	tap		*t = (tap *) tp;
	int		err;
	string_t	**snd;
	char		*ptr;
	int		len;
	int		n, m, ep;

	MCHK (t);
	if ((! t) || (! t -> sp))
		return ERR_FATAL;
	switch (t -> typ) {
	default:
	case None:
		V (1, ("Invalid service/terminal type\n"));
		return ERR_FAIL;
	case PG1:
		for (n = 0; field[n]; ++n)
			;
		if (n != 2) {
			V (1, ("PG1 service/terminal type needs exactly two fields\n"));
			return ERR_FAIL;
		}
		break;
	}
	err = ERR_FATAL;
	if (snd = convert_tap (t, field)) {
		err = NO_ERR;
		for (m = 0; snd[m]; ++m) {
			ptr = (char *) snd[m] -> str;
			len = snd[m] -> len;
			for (n = 0; n < t -> n2; ++n)
				if (tty_send (t -> sp, ptr, len) != len)
					V (1, ("Unable to transmit part\n"));
				else {
					setcb (t);
					ep = tty_expect (t -> sp, t -> t3, "\x06\r", 2, "\x15\r", 2, "\x1e\r", 2, "\x1b\x04\r", 3, NULL);
					if ((! t -> pre16) && t -> r)
						show_response (t);
					if (ep == 1) {
						V (1, ("Transmited part\n"));
						break;
					} else if (ep == 2)
						V (1, ("Can't transmit part\n"));
					else if (ep == 3) {
						V (1, ("Can't transmit this message\n"));
						err = ERR_FAIL;
						n = t -> n2;
						break;
					} else if (ep == 4) {
						V (1, ("Logout forced\n"));
						err = ERR_ABORT;
						n = t -> n2;
						break;
					}
				}
			sfree (snd[m]);
			clrcb (t);
			if (n == t -> n2) {
				V (1, ("Unable to send part %d\n", m));
				for (n = m + 1; snd[n]; ++n)
					sfree (snd[n]);
				break;
			}
		}
		if (! snd[m])
			V (1, ("Sent message\n"));
		else if (err == NO_ERR)
			err = ERR_FAIL;
		free (snd);
	} else
		V (1, ("Unable to convert message\n"));
	return err;
}
/*}}}*/
/*{{{	configuration */
void
tap_config (void *tp, void (*logger) (char, char *, ...), Bool pre16)
{
	tap	*t = (tap *) tp;
	
	MCHK (t);
	if (t) {
		t -> logger = logger;
		if (t -> pre16 = pre16)
			tty_suspend_callback (t -> sp, True);
	}
}

void
tap_timeouts (void *tp, int t1, int t2, int t3, int t4, int t5)
{
	tap	*t = (tap *) tp;

	MCHK (t);
	if (t) {
		if (t1 != -1)
			t -> t1 = t1;
		if (t2 != -1)
			t -> t2 = t2;
		if (t3 != -1)
			t -> t3 = t3;
		if (t4 != -1)
			t -> t4 = t4;
		if (t5 != -1)
			t -> t5 = t5;
	}
}

void
tap_retries (void *tp, int n1, int n2, int n3, int licnt, int locnt)
{
	tap	*t = (tap *) tp;

	MCHK (t);
	if (t) {
		if (n1 != -1)
			t -> n1 = n1;
		if (n2 != -1)
			t -> n2 = n2;
		if (n3 != -1)
			t -> n3 = n3;
		if (licnt != -1)
			t -> licnt = licnt;
		if (locnt != -1)
			t -> locnt = locnt;
	}
}

void
tap_set_convtable (void *tp, void *ctab)
{
	tap	*t = (tap *) tp;
	
	MCHK (t);
	if (t) {
		if (t -> ctab)
			cv_free (t -> ctab);
		t -> ctab = ctab;
	}
}

static void
new_convtab (tap *t)
{
	int	n;
	
	if (t -> ctab = cv_new ())
		for (n = 128; n < 256; ++n)
			cv_invalid (t -> ctab, (char_t) n);
}

void
tap_add_convtable (void *tp, void *ctab)
{
	tap	*t = (tap *) tp;
	
	MCHK (t);
	if (t) {
		if (! t -> ctab)
			new_convtab (t);
		if (t -> ctab)
			cv_merge (t -> ctab, ctab, True);
	}
}
/*}}}*/
/*{{{	new/free/etc */
void *
tap_new (void *sp)
{
	tap	*t;
	
	if (t = (tap *) malloc (sizeof (tap))) {
# ifndef	NDEBUG
		t -> magic = MAGIC;
# endif		/* NDEBUG */
		t -> sp = sp;
		new_convtab (t);
		t -> logger = NULL;
		t -> callid = NULL;
		t -> typ = None;
		t -> pre16 = False;
		t -> t1 = 2;
		t -> t2 = 1;
		t -> t3 = 10;
		t -> t4 = 4;
		t -> t5 = 8;
		t -> n1 = 3;
		t -> n2 = 3;
		t -> n3 = 3;
		t -> licnt = 3;
		t -> locnt = 3;
		t -> r = NULL;
		t -> prv = NULL;
		tty_set_line_callback (t -> sp, getresponse, "\r", (void *) t);
		tty_suspend_callback (t -> sp, True);
	}
	return (void *) t;
}

void *
tap_free (void *tp)
{
	tap	*t = (tap *) tp;
	
	MCHK (t);
	if (t) {
		if (t -> ctab)
			cv_free (t -> ctab);
		if (t -> sp)
			tty_set_line_callback (t -> sp, NULL, NULL, NULL);
		if (t -> callid)
			sfree (t -> callid);
		if (t -> r)
			free_resp (t);
		free (t);
	}
	return NULL;
}

int
tap_preinit (void)
{
	return 0;
}

void
tap_postdeinit (void)
{
}
/*}}}*/
