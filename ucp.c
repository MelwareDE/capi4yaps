/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# include	<stdlib.h>
# include	<string.h>
# include	<time.h>
# include	"pager.h"

/*{{{	typedefs */
typedef struct {
# ifndef	NDEBUG
# define	MAGIC		MKMAGIC ('u', 'c', 'p', '\0')
	long	magic;
# endif		/* NDEBUG */
	void	*sp;		/* the serial connection		*/
	void	*ctab;		/* the conversion table			*/
	void	(*logger) (char, char *, ...);
	char	*uline;		/* received input line			*/
	
	string_t
		*callid;	/* caller id				*/
	Bool	xtend;		/* use extended UCP version		*/
	int	send_tout;	/* timeout during sending		*/
	int	send_retry;	/* # of retries to send a message	*/
	int	rds_tout;	/* timeout for status report		*/
	date_t	delay;		/* delay message			*/
	date_t	expire;		/* expire message			*/
	Bool	rds;		/* request delivery status		*/

	int	cnr;		/* current transaction number		*/
}	ucp;

typedef struct {
	char	*AdC;
	char	*OAdC;
	char	*PID;
	char	*na;
	char	*MT;
	char	*Msg;

	char		*adc;
	char		*oadc;
	int		pid;
	int		mt;
	string_t	*msg;
}	bsimple;

typedef struct {
	char	*AdC;
	char	*OAdC;
	char	*AC;
	char	*NRq;
	char	*NAdC;
	char	*NT;
	char	*NPID;
	char	*na1;
	char	*na2;
	char	*na3;
	char	*DD;
	char	*DDT;
	char	*VP;
	char	*RPID;
	char	*SCTS;
	char	*DSt;
	char	*Rsn;
	char	*DSCTS;
	char	*MT;
	char	*NB;
	char	*Msg;
	char	*MMS;
	char	*na4;
	char	*DCS;
	char	*MCL;
	char	*RPI;
	char	*na5;
	char	*na6;
	char	*res1;
	char	*res2;
	char	*res3;
	char	*res4;
	char	*res5;
	
	char		*adc;
	char		*oadc;
	char		*ac;
	Bool		nrq;
	char		*nadc;
	int		nt;
	int		npid;
	Bool		dd;
	date_t		ddt;
	date_t		vp;
	int		rpid;
	date_t		scts;
	int		dst;
	int		rsn;
	date_t		dscts;
	int		mt;
	int		nb;
	string_t	*msg;
	Bool		mms;
	Bool		dcs;
	int		mcl;
	int		rpi;
}	bextend;

typedef struct {
	char	*Res;
	char	*MVP;
	char	*EC;
	char	*MSG;

	Bool	ack;
	date_t	mvp;
	int	ec;
	char	*adc;
	date_t	scts;
}	banswer;

typedef struct {
	int	trn;
	int	len;
	char	ttyp;
	int	ot;
	char	*data;
	int	cnt;
	union {
		bsimple	s;
		bextend	e;
		banswer	a;
	}	b;
	int	chksum;
}	frame;
/*}}}*/
/*{{{	support routines */
static char_t
hex (int val)
{
	switch (val) {
	default:
	case 0:		return (char_t) '0';
	case 1:		return (char_t) '1';
	case 2:		return (char_t) '2';
	case 3:		return (char_t) '3';
	case 4:		return (char_t) '4';
	case 5:		return (char_t) '5';
	case 6:		return (char_t) '6';
	case 7:		return (char_t) '7';
	case 8:		return (char_t) '8';
	case 9:		return (char_t) '9';
	case 10:	return (char_t) 'A';
	case 11:	return (char_t) 'B';
	case 12:	return (char_t) 'C';
	case 13:	return (char_t) 'D';
	case 14:	return (char_t) 'E';
	case 15:	return (char_t) 'F';
	}
}

static int
unhex (char ch)
{
	switch (ch) {
	default:
	case '0':	return 0;
	case '1':	return 1;
	case '2':	return 2;
	case '3':	return 3;
	case '4':	return 4;
	case '5':	return 5;
	case '6':	return 6;
	case '7':	return 7;
	case '8':	return 8;
	case '9':	return 9;
	case 'A':
	case 'a':	return 10;
	case 'B':
	case 'b':	return 11;
	case 'C':
	case 'c':	return 12;
	case 'D':
	case 'd':	return 13;
	case 'E':
	case 'e':	return 14;
	case 'F':
	case 'f':	return 15;
	}
}
/*}}}*/
/*{{{	frame basics */
static frame *
new_frame (char ttyp, int ot)
{
	frame	*f;
	bsimple	*s;
	bextend	*e;
	banswer	*a;
	
	if (f = (frame *) malloc (sizeof (frame))) {
		f -> trn = 0;
		f -> len = 0;
		f -> ttyp = ttyp;
		f -> ot = ot;
		f -> data = NULL;
		f -> cnt = 0;
		memset (& f -> b, 0, sizeof (f -> b));
		f -> chksum = 0;
		if (f -> ttyp == 'R')
			switch (f -> ot) {
			case 1:
			case 31:
			case 51:
			case 52:
			case 53:
			case 55:
			case 56:
			case 57:
			case 58:
				a = & f -> b.a;
				a -> Res = NULL;
				a -> MVP = NULL;
				a -> EC = NULL;
				a -> MSG = NULL;

				a -> ack = False;
				dat_clear (& a -> mvp);
				a -> ec = -1;
				a -> adc = NULL;
				dat_clear (& a -> scts);
				break;
			}
		else if (f -> ttyp == 'O')
			switch (f -> ot) {
			case 1:
			case 31:
				s = & f -> b.s;
				s -> AdC = NULL;
				s -> OAdC = NULL;
				s -> PID = NULL;
				s -> na = NULL;
				s -> MT = NULL;
				s -> Msg = NULL;
				
				s -> adc = NULL;
				s -> oadc = NULL;
				s -> pid = -1;
				s -> mt = -1;
				s -> msg = NULL;
				break;
			case 51:
			case 52:
			case 53:
			case 55:
			case 56:
			case 57:
			case 58:
				e = & f -> b.e;
				e -> AdC = NULL;
				e -> OAdC = NULL;
				e -> AC = NULL;
				e -> NRq = NULL;
				e -> NAdC = NULL;
				e -> NT = NULL;
				e -> NPID = NULL;
				e -> na1 = NULL;
				e -> na2 = NULL;
				e -> na3 = NULL;
				e -> DD = NULL;
				e -> DDT = NULL;
				e -> VP = NULL;
				e -> RPID = NULL;
				e -> SCTS = NULL;
				e -> DSt = NULL;
				e -> Rsn = NULL;
				e -> DSCTS = NULL;
				e -> MT = NULL;
				e -> NB = NULL;
				e -> Msg = NULL;
				e -> MMS = NULL;
				e -> na4 = NULL;
				e -> DCS = NULL;
				e -> MCL = NULL;
				e -> RPI = NULL;
				e -> na5 = NULL;
				e -> na6 = NULL;
				e -> res1 = NULL;
				e -> res2 = NULL;
				e -> res3 = NULL;
				e -> res4 = NULL;
				e -> res5 = NULL;
	
				e -> adc = NULL;
				e -> oadc = NULL;
				e -> ac = NULL;
				e -> nrq = False;
				e -> nadc = NULL;
				e -> nt = -1;
				e -> npid = -1;
				e -> dd = False;
				dat_clear (& e -> ddt);
				dat_clear (& e -> vp);
				e -> rpid = -1;
				dat_clear (& e -> scts);
				e -> dst = -1;
				e -> rsn = -1;
				dat_clear (& e -> dscts);
				e -> mt = -1;
				e -> nb = -1;
				e -> msg = NULL;
				e -> mms = False;
				e -> dcs = False;
				e -> mcl = -1;
				e -> rpi = -1;
				break;
			}
	}
	return f;
}

static void
free_body (frame *f)
{
# define	dfr(xxx)	if (xxx) free (xxx)	
	bsimple	*s;
	bextend	*e;
	banswer	*a;
	
	s = NULL;
	e = NULL;
	a = NULL;
	if (f -> ttyp == 'O')
		switch (f -> ot) {
		case 1:
		case 31:
			s = & f -> b.s;
			break;
		case 51:
		case 52:
		case 53:
		case 55:
		case 56:
		case 57:
		case 58:
			e = & f -> b.e;
			break;
		}
	else if (f -> ttyp == 'R')
		switch (f -> ot) {
		case 1:
		case 31:
		case 51:
		case 52:
		case 53:
		case 55:
		case 56:
		case 57:
		case 58:
			a = & f -> b.a;
			break;
		}
	if (s) {
		dfr (s -> AdC);
		dfr (s -> OAdC);
		dfr (s -> PID);
		dfr (s -> na);
		dfr (s -> MT);
		dfr (s -> Msg);
		dfr (s -> adc);
		dfr (s -> oadc);
		sfree (s -> msg);
	} else if (e) {
		dfr (e -> AdC);
		dfr (e -> OAdC);
		dfr (e -> AC);
		dfr (e -> NRq);
		dfr (e -> NAdC);
		dfr (e -> NT);
		dfr (e -> NPID);
		dfr (e -> na1);
		dfr (e -> na2);
		dfr (e -> na3);
		dfr (e -> DD);
		dfr (e -> DDT);
		dfr (e -> VP);
		dfr (e -> RPID);
		dfr (e -> SCTS);
		dfr (e -> DSt);
		dfr (e -> Rsn);
		dfr (e -> DSCTS);
		dfr (e -> MT);
		dfr (e -> NB);
		dfr (e -> Msg);
		dfr (e -> MMS);
		dfr (e -> na4);
		dfr (e -> DCS);
		dfr (e -> MCL);
		dfr (e -> RPI);
		dfr (e -> na5);
		dfr (e -> na6);
		dfr (e -> res1);
		dfr (e -> res2);
		dfr (e -> res3);
		dfr (e -> res4);
		dfr (e -> res5);

		dfr (e -> adc);
		dfr (e -> oadc);
		dfr (e -> ac);
		dfr (e -> nadc);
		sfree (e -> msg);
	} else if (a) {
		dfr (a -> Res);
		dfr (a -> MVP);
		dfr (a -> EC);
		dfr (a -> MSG);
		dfr (a -> adc);
	}
# undef		dfr	
}
	
static void
free_frame (frame *f)
{
	if (f) {
		if (f -> data)
			free (f -> data);
		free_body (f);
		free (f);
	}
}
/*}}}*/
/*{{{	assemble */
static char *
encode (ucp *u, string_t *s, Bool docv)
{
	char	*ret;
	char_t	ch;
	int	c;
	int	len;
	int	n, m;

	len = s ? (s -> len * 2) : 0;
	if (ret = malloc (len + 2)) {
		for (n = 0, m = 0; n < len; ++m) {
			if (docv) {
				if ((c = cv_conv (u -> ctab, s -> str[m])) < 0)
					continue;
				ch = (char_t) c;
			} else
				ch = s -> str[m];
			ret[n++] = hex ((ch >> 4) & 0xf);
			ret[n++] = hex (ch & 0xf);
		}
		ret[n] = '\0';
	}
	return ret;
}

static inline void
gbool (char **ptr, Bool what, char *t, char *f)
{
	if (*ptr)
		free (*ptr);
	if (what)
		*ptr = t ? strdup (t) : NULL;
	else
		*ptr = f ? strdup (f) : NULL;
}

static inline void
gdate (char **ptr, date_t *d, int len)
{
	if (*ptr)
		free (*ptr);
	if (d -> day > 0) {
		if (*ptr = malloc (14)) {
			sprintf (*ptr, "%02d%02d%02d%02d%02d%02d",
				 d -> day, d -> mon, d -> year % 100,
				 d -> hour, d -> min, d -> sec);
			if (len < 12)
				(*ptr)[len] = '\0';
		}
	} else
		*ptr = NULL;
}

static inline void
gint (char **ptr, int val, char *fmt, int def)
{
	if (*ptr)
		free (*ptr);
	if (val == -1)
		val = def;
	if (val != -1) {
		if (*ptr = malloc (32))
			sprintf (*ptr, (fmt ? fmt : "%d"), val);
	} else
		*ptr = NULL;
}

static inline void
gstr (char **ptr, char *str)
{
	if (*ptr)
		free (*ptr);
	*ptr = str ? strdup (str) : NULL;
}

static string_t *
assemble_frame (ucp *u, frame *f)
{
	bsimple		*s;
	bextend		*e;
	banswer		*a;
	string_t	*ret;
	Bool		fail;
	char		buf[64], *ptr;
	int		len, n;
	unsigned long	chk;
	
	if (! (ret = snew (NULL, 128)))
		return NULL;
	if (! (scopyc (ret, "\x02") && scatc (ret, "00/00000/")))
		return sfree (ret);
	sprintf (buf, "%c/%02d/", f -> ttyp, f -> ot);
	if (! scatc (ret, buf))
		return sfree (ret);
	fail = False;
	if (f -> ttyp == 'R') {
		a = & f -> b.a;
		switch (f -> ot) {
		default:
			fail = True;
			break;
		case 1:
		case 31:
		case 51:
		case 52:
		case 53:
		case 55:
		case 56:
		case 57:
		case 58:
			gbool (& a -> Res, a -> ack, "A", "N");
			if (a -> MSG) {
				free (a -> MSG);
				a -> MSG = NULL;
			}
			if (a -> ack) {
				gdate (& a -> MVP, & a -> mvp, 10);
				ptr = NULL;
				gdate (& ptr, & a -> scts, 12);
				if (ptr) {
					if (a -> adc) {
						if (a -> MSG = malloc (strlen (ptr) + strlen (a -> adc) + 4))
							sprintf (a -> MSG, "%s:%s", a -> adc, ptr);
						free (ptr);
					} else
						a -> MSG = ptr;
				} else if (a -> adc)
					a -> MSG = strdup (a -> adc);
			} else
				gint (& a -> EC, a -> ec, NULL, 3);
			if (((f -> ot == 1) || (f -> ot == 31)) && a -> ack) {
				if (! (scatc (ret, a -> Res) && scatc (ret, "/") &&
				       scatc (ret, a -> MSG) && scatc (ret, "/")))
					fail = True;
			} else if (! (scatc (ret, a -> Res) && scatc (ret, "/") &&
				      scatc (ret, (a -> ack ? a -> MVP : a -> EC)) && scatc (ret, "/") &&
				      scatc (ret, a -> MSG) && scatc (ret, "/")))
				fail = True;
			break;
		}
	} else if (f -> ttyp == 'O') {
		s = & f -> b.s;
		e = & f -> b.e;
		switch (f -> ot) {
		default:
			fail = True;
			break;
		case 1:
			gstr (& s -> AdC, s -> adc);
			gstr (& s -> OAdC, s -> oadc);
			gstr (& s -> na, NULL);
			gint (& s -> MT, s -> mt, NULL, 3);
			if (s -> Msg)
				free (s -> Msg);
			s -> Msg = encode (u, s -> msg, True);
			if (! (scatc (ret, s -> AdC) && scatc (ret, "/") &&
			       scatc (ret, s -> OAdC) && scatc (ret, "/") &&
			       scatc (ret, s -> na) && scatc (ret, "/") &&
			       scatc (ret, s -> MT) && scatc (ret, "/") &&
			       scatc (ret, s -> Msg) && scatc (ret, "/")))
				fail = True;
			break;
		case 31:
			gstr (& s -> AdC, s -> adc);
			gint (& s -> PID, s -> pid, "%04d", -1);
			if (! (scatc (ret, s -> AdC) && scatc (ret, "/") &&
			       scatc (ret, s -> PID) && scatc (ret, "/")))
				fail = True;
			break;
		case 51:
		case 52:
		case 53:
		case 55:
		case 56:
		case 57:
		case 58:
			gstr (& e -> AdC, e -> adc);
			gstr (& e -> OAdC, e -> oadc);
			gstr (& e -> AC, e -> ac);
			gbool (& e -> NRq, e -> nrq, "1", NULL);
			gstr (& e -> NAdC, (e -> nrq ? e -> nadc : NULL));
			gint (& e -> NT, (e -> nrq ? e -> nt : -1), NULL, -1);
			gint (& e -> NPID, (e -> nrq ? e -> npid : -1), "%04d", -1);
			gstr (& e -> na1, NULL);
			gstr (& e -> na2, NULL);
			gstr (& e -> na3, NULL);
			gbool (& e -> DD, e -> dd, "1", NULL);
			if (e -> dd)
				gdate (& e -> DDT, & e -> ddt, 10);
			else
				gstr (& e -> DDT, NULL);
			gdate (& e -> VP, & e -> vp, 10);
			gint (& e -> RPID, e -> rpid, "%04d", -1);
			gdate (& e -> SCTS, & e -> scts, 12);
			gint (& e -> DSt, e -> dst, NULL, -1);
			gint (& e -> Rsn, e -> rsn, "%03d", -1);
			gdate (& e -> DSCTS, & e -> dscts, 12);
			gint (& e -> MT, e -> mt, NULL, 3);
			if (e -> mt == 4)
				if (e -> nb != -1)
					len = e -> nb;
				else
					len = e -> msg ? e -> msg -> len * 8 : 0;
			else
				len = -1;
			gint (& e -> NB, len, NULL, -1);
			if (e -> Msg)
				free (e -> Msg);
			e -> Msg = encode (u, e -> msg, (e -> mt == 4 ? False : True));
			gbool (& e -> MMS, e -> mms, "1", NULL);
			gstr (& e -> na4, NULL);
			gbool (& e -> DCS, e -> dcs, "1", NULL);
			gint (& e -> MCL, e -> mcl, NULL, -1);
			gint (& e -> RPI, e -> rpi, NULL, -1);
			gstr (& e -> na5, NULL);
			gstr (& e -> na6, NULL);
			gstr (& e -> res1, NULL);
			gstr (& e -> res2, NULL);
			gstr (& e -> res3, NULL);
			gstr (& e -> res4, NULL);
			gstr (& e -> res5, NULL);
			if (! (scatc (ret, e -> AdC) && scatc (ret, "/") &&
			       scatc (ret, e -> OAdC) && scatc (ret, "/") &&
			       scatc (ret, e -> AC) && scatc (ret, "/") &&
			       scatc (ret, e -> NRq) && scatc (ret, "/") &&
			       scatc (ret, e -> NAdC) && scatc (ret, "/") &&
			       scatc (ret, e -> NT) && scatc (ret, "/") &&
			       scatc (ret, e -> NPID) && scatc (ret, "/") &&
			       scatc (ret, e -> na1) && scatc (ret, "/") &&
			       scatc (ret, e -> na2) && scatc (ret, "/") &&
			       scatc (ret, e -> na3) && scatc (ret, "/") &&
			       scatc (ret, e -> DD) && scatc (ret, "/") &&
			       scatc (ret, e -> DDT) && scatc (ret, "/") &&
			       scatc (ret, e -> VP) && scatc (ret, "/") &&
			       scatc (ret, e -> RPID) && scatc (ret, "/") &&
			       scatc (ret, e -> SCTS) && scatc (ret, "/") &&
			       scatc (ret, e -> DSt) && scatc (ret, "/") &&
			       scatc (ret, e -> Rsn) && scatc (ret, "/") &&
			       scatc (ret, e -> DSCTS) && scatc (ret, "/") &&
			       scatc (ret, e -> MT) && scatc (ret, "/") &&
			       scatc (ret, e -> NB) && scatc (ret, "/") &&
			       scatc (ret, e -> Msg) && scatc (ret, "/") &&
			       scatc (ret, e -> MMS) && scatc (ret, "/") &&
			       scatc (ret, e -> na4) && scatc (ret, "/") &&
			       scatc (ret, e -> DCS) && scatc (ret, "/") &&
			       scatc (ret, e -> MCL) && scatc (ret, "/") &&
			       scatc (ret, e -> RPI) && scatc (ret, "/") &&
			       scatc (ret, e -> na5) && scatc (ret, "/") &&
			       scatc (ret, e -> na6) && scatc (ret, "/") &&
			       scatc (ret, e -> res1) && scatc (ret, "/") &&
			       scatc (ret, e -> res2) && scatc (ret, "/") &&
			       scatc (ret, e -> res3) && scatc (ret, "/") &&
			       scatc (ret, e -> res4) && scatc (ret, "/") &&
			       scatc (ret, e -> res5) && scatc (ret, "/")))
				fail = True;
			break;
		}
	} else
		fail = True;
	if (! fail) {
		len = ret -> len - 1 + 2;
		sprintf (buf, "%02d/%05d", f -> trn, len);
		if (sputc (ret, buf, 1, -1)) {
			for (n = 1, chk = 0; n < ret -> len; ++n)
				chk += ret -> str[n] & 0xff;
			if (sexpand (ret, ret -> len + 4)) {
				ret -> str[ret -> len++] = hex ((chk >> 4) & 0xf);
				ret -> str[ret -> len++] = hex (chk & 0xf);
				ret -> str[ret -> len++] = '\x03';
			}
		} else
			fail = True;
	}
	if (fail)
		ret = sfree (ret);
	return ret;
}
/*}}}*/
/*{{{	parse */
static date_t
parse_date (char *str)
{
	date_t		ret;
	int		n;
	int		val;
	time_t		tim;
	struct tm	*tt;

	ret.day = 0;
	ret.mon = 0;
	ret.year = 0;
	ret.hour = 0;
	ret.min = 0;
	ret.sec = 0;
	if (str)
		for (n = 0; (n < 6) && *str && *(str + 1); ++n) {
			val = unhex (*str) * 10 + unhex (*(str + 1));
			str += 2;
			switch (n) {
			case 0:	ret.day = val;	break;
			case 1:	ret.mon = val;	break;
			case 2:	ret.year = val;	break;
			case 3:	ret.hour = val;	break;
			case 4:	ret.min = val;	break;
			case 5:	ret.sec = val;	break;
			}
		}
	time (& tim);
	if (tt = localtime (& tim))
		ret.year += ((tt -> tm_year + 1900) / 100) * 100;
	return ret;
}

static string_t *
parse_message (ucp *u, int mt, char *msg)
{
	string_t	*ret;
	int		len;
	int		n;
	char_t		ch;
	void		*ct;
	
	if ((mt != 4) && (u -> ctab))
		ct = cv_reverse (u -> ctab);
	else
		ct = NULL;
	len = strlen (msg);
	if (ret = snew (NULL, len / 2 + 2))
		for (n = 0; n + 1 < len; n += 2) {
			ch = (unhex (msg[n]) << 4) | unhex (msg[n + 1]);
			ret -> str[ret -> len++] = cv_conv (ct, ch);
		}
	if (ct)
		cv_free (ct);
	return ret;
}

static Bool
parse_body (ucp *u, frame *f)
{
	Bool	ret;
	char	*dat, *ptr, *sav;
	char	**rev;
	Bool	done, fail;
	int	n;
	
	if (! (dat = strdup (f -> data)))
		return False;
	ret = False;
	done = False;
	fail = False;
	for (ptr = dat, n = 0; (! done) && (! fail) && ptr; ++n) {
		sav = ptr;
		if (ptr = strchr (ptr, '/'))
			*ptr++ = '\0';
		rev = NULL;
		if (f -> ttyp == 'O') {
			switch (f -> ot) {
			case 1:
				switch (n) {
				case 0:		rev = & f -> b.s.AdC;	break;
				case 1:
					rev = & f -> b.s.OAdC;
					f -> b.s.adc = f -> b.s.AdC ? strdup (f -> b.s.AdC) : NULL;
					break;
				case 2:	
					rev = & f -> b.s.na;
					f -> b.s.oadc = f -> b.s.OAdC ? strdup (f -> b.s.OAdC) : NULL;
					break;
				case 3:		rev = & f -> b.s.MT;	break;
				case 4:
					rev = & f -> b.s.Msg;
					f -> b.s.mt = f -> b.s.MT ? atoi (f -> b.s.MT) : -1;
					done = True;
					break;
				}
				break;
			case 31:
				switch (n) {
				case 0:		rev = & f -> b.s.AdC;	break;
				case 1:	
					rev = & f -> b.s.PID;
					f -> b.s.adc = f -> b.s.AdC ? strdup (f -> b.s.AdC) : NULL;
					done = True;
					break;
				}
				break;
			case 51:
			case 52:
			case 53:
			case 55:
			case 56:
			case 57:
			case 58:
				switch (n) {
				case 0:		rev = & f -> b.e.AdC;	break;
				case 1:
					rev = & f -> b.e.OAdC;
					f -> b.e.adc = f -> b.e.AdC ? strdup (f -> b.e.AdC) : NULL;
					break;
				case 2:
					rev = & f -> b.e.AC;
					f -> b.e.oadc = f -> b.e.OAdC ? strdup (f -> b.e.OAdC) : NULL;
					break;
				case 3:	
					rev = & f -> b.e.NRq;
					f -> b.e.ac = f -> b.e.AC ? strdup (f -> b.e.AC) : NULL;
					break;
				case 4:
					rev = & f -> b.e.NAdC;
					if (f -> b.e.NRq)
						f -> b.e.nrq = (atoi (f -> b.e.NRq) == 1 ? True : False);
					else
						f -> b.e.nrq = False;
					break;
				case 5:	
					rev = & f -> b.e.NT;
					f -> b.e.nadc = f -> b.e.NAdC ? strdup (f -> b.e.NAdC) : NULL;
					break;
				case 6:
					rev = & f -> b.e.NPID;
					f -> b.e.nt = f -> b.e.NT ? atoi (f -> b.e.NT) : 0;
					break;
				case 7:
					rev = & f -> b.e.na1;
					f -> b.e.npid = f -> b.e.NPID ? atoi (f -> b.e.NPID) : -1;
					break;
				case 8:		rev = & f -> b.e.na2;	break;
				case 9:		rev = & f -> b.e.na3;	break;
				case 10:	rev = & f -> b.e.DD;	break;
				case 11:
					rev = & f -> b.e.DDT;
					if (f -> b.e.DD)
						f -> b.e.dd = (atoi (f -> b.e.DD) == 1 ? True : False);
					else
						f -> b.e.dd = False;
					break;
				case 12:
					rev = & f -> b.e.VP;
					f -> b.e.ddt = parse_date (f -> b.e.DDT);
					break;
				case 13:
					rev = & f -> b.e.RPID;
					f -> b.e.vp = parse_date (f -> b.e.VP);
					break;
				case 14:
					rev = & f -> b.e.SCTS;
					f -> b.e.rpid = f -> b.e.RPID ? atoi (f -> b.e.RPID) : -1;
					break;
				case 15:
					rev = & f -> b.e.DSt;
					f -> b.e.scts = parse_date (f -> b.e.SCTS);
					break;
				case 16:
					rev = & f -> b.e.Rsn;
					f -> b.e.dst = f -> b.e.DSt ? atoi (f -> b.e.DSt) : -1;
					break;
				case 17:
					rev = & f -> b.e.DSCTS;
					f -> b.e.rsn = f -> b.e.Rsn ? atoi (f -> b.e.Rsn) : 0;
					break;
				case 18:
					rev = & f -> b.e.MT;
					f -> b.e.dscts = parse_date (f -> b.e.DSCTS);
					break;
				case 19:
					rev = & f -> b.e.NB;
					f -> b.e.mt = f -> b.e.MT ? atoi (f -> b.e.MT) : -1;
					break;
				case 20:
					rev = & f -> b.e.Msg;
					f -> b.e.nb = f -> b.e.NB ? atoi (f -> b.e.NB) : 0;
					break;
				case 21:
					rev = & f -> b.e.MMS;
					if (f -> b.e.Msg)
						if (! (f -> b.e.msg = parse_message (u, f -> b.e.mt, f -> b.e.Msg)))
							fail = True;
					break;
				case 22:
					rev = & f -> b.e.na4;
					if (f -> b.e.MMS)
						f -> b.e.mms = (atoi (f -> b.e.MMS) == 1 ? True : False);
					else
						f -> b.e.mms = False;
					break;
				case 23:	rev = & f -> b.e.DCS;	break;
				case 24:
					rev = & f -> b.e.MCL;
					if (f -> b.e.DCS)
						f -> b.e.dcs = (atoi (f -> b.e.DCS) == 1 ? True : False);
					else
						f -> b.e.dcs = False;
					break;
				case 25:
					rev = & f -> b.e.RPI;
					f -> b.e.mcl = f -> b.e.MCL ? atoi (f -> b.e.MCL) : -1;
					break;
				case 26:
					rev = & f -> b.e.na5;
					f -> b.e.rpi = f -> b.e.RPI ? atoi (f -> b.e.RPI) : -1;
					break;
				case 27:	rev = & f -> b.e.na6;	break;
				case 28:	rev = & f -> b.e.res1;	break;
				case 29:	rev = & f -> b.e.res2;	break;
				case 30:	rev = & f -> b.e.res3;	break;
				case 31:	rev = & f -> b.e.res4;	break;
				case 32:
					rev = & f -> b.e.res5;
					done = True;
					break;
				}
				break;
			}
		} else if (f -> ttyp == 'R') {
			switch (f -> ot) {
			case 1:
			case 31:
				switch (n) {
				case 0:		rev = & f -> b.a.Res;	break;
				case 1:
					if (f -> b.a.Res) {
						if (f -> b.a.Res[0] == 'A') {
							f -> b.a.ack = True;
							rev = & f -> b.a.MSG;
							done = True;
						} else if (f -> b.a.Res[0] == 'N') {
							f -> b.a.ack = False;
							rev = & f -> b.a.EC;
						} else
							fail = True;
					}
					break;
				case 2:
					if (! f -> b.a.ack) {
						f -> b.a.ec = f -> b.a.EC ? atoi (f -> b.a.EC) : -1;
						rev = & f -> b.a.MSG;
						done = True;
					}
					break;
				}
				break;
			case 51:
			case 52:
			case 53:
			case 55:
			case 56:
			case 57:
			case 58:
				switch (n) {
				case 0:		rev = & f -> b.a.Res;	break;
				case 1:
					if (f -> b.a.Res) {
						if (f -> b.a.Res[0] == 'A') {
							f -> b.a.ack = True;
							rev = & f -> b.a.MVP;
						} else if (f -> b.a.Res[0] == 'N') {
							f -> b.a.ack = False;
							rev = & f -> b.a.EC;
						} else
							fail = True;
					} else
						fail = True;
					break;
				case 2:
					if (f -> b.a.ack)
						f -> b.a.mvp = parse_date (f -> b.a.MVP);
					else
						f -> b.a.ec = f -> b.a.EC ? atoi (f -> b.a.EC) : -1;
					rev = & f -> b.a.MSG;
					done = True;
					break;
				}
				break;
			}
		} else
			fail = True;
		if (! fail) {
			if (! rev)
				fail = True;
			else {
				if (*sav) {
					if (! (*rev = strdup (sav)))
						fail = True;
				} else
					*rev = NULL;
			}
			if ((! ptr) && (! done))
				fail = True;
		}
	}
	if (done && (! fail) && (! ptr)) {
		if (f -> ttyp == 'O') {
			switch (f -> ot) {
			case 1:
			case 31:
				if (f -> b.s.Msg)
					if (! (f -> b.s.msg = parse_message (u, f -> b.s.mt, f -> b.s.Msg)))
						fail = True;
				break;
			}
		} else if (f -> ttyp == 'R') {
			switch (f -> ot) {
			case 1:
			case 31:
			case 51:
			case 52:
			case 53:
			case 55:
			case 56:
			case 57:
			case 58:
				if (f -> b.a.MSG) {
					if (! (f -> b.a.adc = strdup (f -> b.a.MSG)))
						fail = True;
					else {
						if (ptr = strchr (f -> b.a.adc, ':'))
							*ptr++ = '\0';
						f -> b.a.scts = parse_date (ptr);
					}
				}
				break;
			}
		} else
			fail = True;
		if (! fail)
			ret = True;
	}
	free (dat);
	return ret;
}

static frame *
parse_frame (ucp *u, char *str)
{
	frame		*f;
	char		*start, *end;
	char		*ptr, *sav;
	int		n, len;
	unsigned int	chksum;
	
	f = NULL;
	if (str = strdup (str)) {
		start = strchr (str, '\x02');
		end = strchr (str, '\x03');
		ptr = strrchr (str, '/');
		if (start && end && (end > start) &&
		    ptr && (start + 1 < ptr) && (ptr + 3 == end) &&
		    (f = new_frame ('\0', 0))) {
			memset (f, 0, sizeof (frame));
			len = (int) ((unsigned long) end - (unsigned long) start) - 1;
			for (chksum = 0, n = 1; n < len - 1; ++n)
				chksum += (unsigned char) start[n];
			chksum &= 0xff;
			ptr = start + 1;
			for (n = 0; n < 4; ++n) {
				if (! (sav = ptr))
					break;
				if (ptr = strchr (ptr, '/'))
					*ptr++ = '\0';
				switch (n) {
				case 0:
					f -> trn = atoi (sav);
					break;
				case 1:
					f -> len = atoi (sav);
					break;
				case 2:
					f -> ttyp = *sav;
					break;
				case 3:
					f -> ot = atoi (sav);
					break;
				}
			}
			sav = ptr;
			if ((n < 4) || (f -> len != len) || (! sav) || (! (ptr = strrchr (sav, '/')))) {
				free (f);
				f = NULL;
			} else {
				f -> cnt = (int) ((unsigned long) ptr - (unsigned long) sav);
				++ptr;
				if (*ptr && *(ptr + 1))
					f -> chksum = (unhex (*ptr) << 4) | unhex (*(ptr + 1));
				else
					f -> chksum = -1;
				if ((chksum == f -> chksum) && (f -> data = malloc (f -> cnt + 1))) {
					memcpy (f -> data, sav, f -> cnt);
					f -> data[f -> cnt] = '\0';
					if (! parse_body (u, f)) {
						free_frame (f);
						f = NULL;
					}
				} else {
					free (f);
					f = NULL;
				}
			}
		}
		free (str);
	}
	return f;
}
/*}}}*/
/*{{{	convert */
static string_t *
convert_ucp (ucp *u, Bool last, string_t *pagerid, string_t *msg)
{
	string_t	*ret;
	frame		*f;
	int		ot;
	int		n;
	int		ch;
	char		*adc, *oadc;
	
	ret = NULL;
	if (! u -> xtend)
		ot = 1;
	else
		ot = 51;
	if (f = new_frame ('O', ot)) {
		f -> trn = u -> cnr++;
		adc = sextract (pagerid);
		oadc = u -> callid ? sextract (u -> callid) : NULL;
		if (msg)
			msg = snew (msg -> str, msg -> len);
		if (u -> xtend) {
			f -> b.e.adc = adc;
			f -> b.e.oadc = oadc;
			if (u -> rds && (u -> delay.day <= 0)) {
				f -> b.e.nrq = True;
				f -> b.e.nt = 7;
			} else
				f -> b.e.nrq = False;
			if (u -> delay.day > 0) {
					f -> b.e.dd = True;
				f -> b.e.ddt = u -> delay;
			} else
				f -> b.e.dd = False;
			if (u -> expire.day > 0)
				f -> b.e.vp = u -> expire;
			for (n = 0; n < msg -> len; ++n)
				if ((ch = cv_conv (u -> ctab, msg -> str[n])) != -1)
					if (ch & 0x80)
						break;
			if (n < msg -> len) {
				f -> b.e.mt = 4;
				f -> b.e.nb = msg -> len * 8;
			} else
				f -> b.e.mt = 3;
			f -> b.e.msg = msg;
		} else {
			f -> b.s.adc = adc;
			f -> b.s.oadc = oadc;
			f -> b.s.mt = 3;
			f -> b.s.msg = msg;
		}
		ret = assemble_frame (u, f);
		free_frame (f);
	}
	return ret;
}
/*}}}*/
/*{{{	callback interface */
static void
grabline (void *sp, string_t *s, char_t sep, void *data)
{
	ucp	*u = (ucp *) data;
	char	*str;

	MCHK (u);
	if (u) {
		if (u -> uline) {
			free (u -> uline);
			u -> uline = NULL;
		}
		if (str = sextract (s)) {
			if (u -> uline = malloc (strlen (str) + 4))
				sprintf (u -> uline, "%s%c", str, (char) sep);
			free (str);
		}
	}
}
/*}}}*/
/*{{{	interpret originate message */
static void
interpret_originate (ucp *u, frame *f)
{
	string_t	*msg;
	
	msg = NULL;
	switch (f -> ot) {
	case 1:
	case 31:
		msg = f -> b.s.msg;
		break;
	case 51:
	case 52:
	case 53:
	case 55:
	case 56:
	case 57:
	case 58:
		msg = f -> b.e.msg;
		break;
	}
	if (msg)
		V (1, ("Got message type %d: `%s'\n", f -> ot, schar (msg)));
}
/*}}}*/
/*{{{	send a message */
static Bool
send_msg (ucp *u, string_t *msg, Bool wds, int *err)
{
	Bool		ret;
	Bool		dosend;
	int		n;
	int		ep;
	frame		*f, *ans;
	banswer		*an;
	string_t	*astr;
	char		*ptr;

	ret = False;
	dosend = True;
	for (n = 0; n < 2; ++n) {
		if ((! n) && dosend) {
			dosend = False;
			if (tty_send (u -> sp, msg -> str, msg -> len) != msg -> len) {
				V (1, ("Unable to send message\n"));
				*err = ERR_FAIL;
				break;
			}
		}
		if (n && (! wds))
			continue;
		ep = tty_expect (u -> sp, (n ? u -> rds_tout : u -> send_tout), "\x03", 1, NULL);
		if ((ep != 1) || (! u -> uline)) {
			if (ep < 0)
				*err = ERR_FAIL;
			break;
		}
		if (f = parse_frame (u, u -> uline)) {
			switch (f -> ttyp) {
			case 'R':
				if (f -> b.a.ack) {
					V (1, ("Message spooled\n"));
					ret = True;
					break;
				} else {
					switch (f -> b.a.ec) {
					default:
						ptr = "unknown error";
						break;
					case 1:
						ptr = "checksum error";
						break;
					case 2:
						ptr = "syntax error";
						break;
					case 3:
						ptr = "operation not supported by system";
						break;
					case 4:
						ptr = "operation not allowed (at this point)";
						break;
					case 5:
						ptr = "call barring active";
						break;
					case 6:
						ptr = "AdC invalid";
						break;
					case 7:
						ptr = "authentication failure";
						break;
					case 8:
						ptr = "legitimisation code for all calls, failure";
						break;
					case 9:
						ptr = "GA not valid";
						break;
					case 10:
						ptr = "repetition not allowed";
						break;
					case 11:
						ptr = "legitimisation code for repetition, failure";
						break;
					case 12:
						ptr = "priority call not allowed";
						break;
					case 13:
						ptr = "legitimisation code for priority, failure";
						break;
					case 14:
						ptr = "urgent message not allowed";
						break;
					case 15:
						ptr = "legitimisation code for urgent message, failure";
						break;
					case 16:
						ptr = "reveerse charging not allowed";
						break;
					case 17:
						ptr = "legitimisation code for reverse charging, failure";
						break;
					case 18:
						ptr = "deferred delivery not allowed";
						break;
					case 19:
						ptr = "new AC not valid";
						break;
					case 20:
						ptr = "new legitimisation code not valid";
						break;
					case 21:
						ptr = "standard text not valid";
						break;
					case 22:
						ptr = "time period not valid";
						break;
					case 23:
						ptr = "message type not supported by system";
						break;
					case 24:
						ptr = "message too long";
						break;
					case 25:
						ptr = "requested standard text not valid";
						break;
					case 26:
						ptr = "message type not valid for pager type";
						break;
					case 27:
						ptr = "message not found in SMSC";
						break;
					case 30:
						ptr = "subscriber hang-up";
						break;
					case 31:
						ptr = "fax group not supported";
						break;
					case 32:
						ptr = "fax message type not supported";
						break;
					case 33:
						ptr = "address already in list (60 series)";
						break;
					case 34:
						ptr = "address not in list (60 series)";
						break;
					case 35:
						ptr = "list full (60 series)";
						break;
					}
					V (1, ("Invalid message (error %d) %s\n", f -> b.a.ec, (ptr ? ptr : "")));
					switch (f -> b.a.ec) {
					case -1:
					case 1:		case 2:		case 3:
					case 6:		case 7:		case 8:
					case 9:		case 10:	case 11:
					case 12:	case 13:	case 14:
					case 15:	case 16:	case 17:
					case 18:	case 19:	case 20:
					case 21:	case 22:	case 23:
					case 24:	case 25:	case 26:
					case 27:	case 31:	case 32:
					case 33:	case 34:	case 35:
						*err = ERR_FAIL;
						break;
					default:
						*err = ERR_FATAL;
						break;
					}
				}
				break;
			case 'O':
				if (n && wds && (f -> ot == 53)) {
					if (u -> logger) {
						switch (f -> b.e.dst) {
						default:	ptr = NULL;	break;
						case 0:	ptr = "delivered";	break;
						case 1:	ptr = "buffered";	break;
						}
						if (ptr)
							(*u -> logger) (LG_PROTO, "UCP: message %s", ptr);
					}
					printf ("Message ");
					switch (f -> b.e.dst) {
					case 0:	printf ("delivered");		break;
					case 1:	printf ("buffered");		break;
					case 2:	printf ("not delivered");	break;
					default:
						printf ("failed");
						break;
					}
					printf (" (%d)", f -> b.e.rsn);
					switch (f -> b.e.rsn) {
					default:
						ptr = "unknown error";
						break;
					case 0x01:	
						ptr = "successful delivered";
						break;
					case 0x02: case 0x03: case 0x04:
					case 0x05: case 0x06: case 0x07:
						ptr = "temporary no service";
						break;
					case 0x09:
						ptr = "unknown service";
						break;
					case 0x0a:
						ptr = "network timeout";
						break;
					case 0x32:
						ptr = "storing time expired";
						break;
					case 0x64:
						ptr = "service not supported";
						break;
					case 0x65:
						ptr = "receiver unknown";
						break;
					case 0x66:
						ptr = "service not available";
						break;
					case 0x67:
						ptr = "call locked";
						break;
					case 0x68:
						ptr = "operation locked";
						break;
					case 0x69:
						ptr = "service center overrun";
						break;
					case 0x6a:
						ptr = "service not supported";
						break;
					case 0x6b:
						ptr = "receiver temporary not reachable";
						break;
					case 0x6c:
						ptr = "delivering error";
						break;
					case 0x6d:
						ptr = "receiver run out of memory";
						break;
					case 0x6e:
						ptr = "protocol error";
						break;
					case 0x6f:
						ptr = "receiver does not support service";
						break;
					case 0x70:
						ptr = "unknown serice center";
						break;
					case 0x71:
						ptr = "service center overrun";
						break;
					case 0x72:
						ptr = "illegal receiving device";
						break;
					case 0x73:
						ptr = "receiver no customer";
						break;
					case 0x74:
						ptr = "error in receiving device";
						break;
					case 0x75:
						ptr = "lower protocol not available";
						break;
					case 0x76:
						ptr = "system error";
						break;
					case 0x77:
						ptr = "PLMN system error";
						break;
					case 0x78:
						ptr = "HLR system error";
						break;
					case 0x79:
						ptr = "VLR system error";
						break;
					case 0x7a:
						ptr = "previous VLR system error";
						break;
					case 0x7b:
						ptr = "error on delivering (check receiver ID)";
						break;
					case 0x7c:
						ptr = "VMSC system error";
						break;
					case 0x7d:
						ptr = "EIR system error";
						break;
					case 0x7e:
						ptr = "system error";
						break;
					case 0x7f:
						ptr = "unexpected data";
						break;
					case 0xc8:
						ptr = "addressing error for service center";
						break;
					case 0xc9:
						ptr = "invalid absolute storing time";
						break;
					case 0xca:
						ptr = "message too large";
						break;
					case 0xcb:
						ptr = "GDM message cannot be extracted";
						break;
					case 0xcc:
						ptr = "translation into IA5 not possible";
						break;
					case 0xcd:
						ptr = "invalid format of storing time";
						break;
					case 0xce:
						ptr = "invalid receiver address";
						break;
					case 0xcf:
						ptr = "message sent twice";
						break;
					case 0xd0:
						ptr = "invalid message type";
						break;
					}
					if (ptr)
						printf (" reason is %s", ptr);
					if (f -> b.e.msg)
						printf (": %s", schar (f -> b.e.msg));
					printf ("\n");
				} else {
					if (! n)
						--n;
					interpret_originate (u, f);
				}
				if (ans = new_frame ('R', f -> ot)) {
					ans -> trn = f -> trn;
					an = & ans -> b.a;
					an -> adc = u -> callid ? sextract (u -> callid) : NULL;
					dat_localtime (& an -> scts);
					switch (f -> ot) {
					case 1:
						an -> ack = False;
						an -> ec = 23;
						break;
					case 31:
						an -> ack = True;
						dosend = True;
						break;
					case 51:
						an -> ack = False;
						an -> ec = 23;
						break;
					case 52:
						an -> ack = True;
						break;
					case 53:
						an -> ack = True;
						break;
					case 55:
						an -> ack = False;
						an -> ec = 23;
						break;
					case 56:
						an -> ack = False;
						an -> ec = 23;
						break;
					case 57:
						an -> ack = True;
						break;
					case 58:
						an -> ack = True;
						break;
					}
					if (astr = assemble_frame (u, ans)) {
						if (tty_send (u -> sp, astr -> str, astr -> len) != astr -> len) {
							V (1, ("Unable to send answer\n"));
							*err = ERR_FAIL;
						}
						sfree (astr);
					}
					free_frame (ans);
				}
				break;
			}
			free_frame (f);
			if (*err != NO_ERR)
				break;
		}
	}
	return ret;
}
/*}}}*/
/*{{{	login/logout/transmit */
int
ucp_login (void *up, string_t *callid)
{
	ucp	*u = (ucp *) up;

	MCHK (u);
	if ((! u) || (! u -> sp))
		return ERR_ABORT;
	u -> cnr = 0;
	u -> callid = sfree (u -> callid);
	if (callid && (! (u -> callid = snew (callid -> str, callid -> len))))
		return ERR_ABORT;
	return NO_ERR;
}

int
ucp_logout (void *up)
{
	MCHK ((ucp *) up);
	return NO_ERR;
}

int
ucp_transmit (void *up, string_t *pagerid, string_t *msg, Bool last)
{
	ucp		*u = (ucp *) up;
	int		n, err;
	Bool		done;
	string_t	*smsg;
	
	MCHK (u);
	if ((! u) || (! u -> sp))
		return ERR_FATAL;
	err = NO_ERR;
	tty_set_line_callback (u -> sp, grabline, "\x03", (void *) u);
	for (n = 0; n < u -> send_retry; ++n)
		if (smsg = convert_ucp (u, last, pagerid, msg)) {
			done = send_msg (u, smsg, (u -> delay.day > 0 ? False : u -> rds), & err);
			sfree (smsg);
			if (done || (err != NO_ERR))
				break;
		}
	if ((n == u -> send_retry) || (err != NO_ERR)) {
		if (err == NO_ERR)
			err = ERR_FAIL;
		V (1, ("Unable to send message\n"));
	}
	tty_set_line_callback (u -> sp, NULL, NULL, NULL);
	if (u -> uline) {
		free (u -> uline);
		u -> uline = NULL;
	}
	return err;
}
/*}}}*/
/*{{{	configuration */
void
ucp_config (void *up, void (*logger) (char, char *, ...),
	    Bool xtend, int stout, int retry, int rtout,
	    date_t *delay, date_t *expire, Bool rds)
{
	ucp	*u = (ucp *) up;
	
	MCHK (u);
	if (u) {
		u -> logger = logger;
		u -> xtend = xtend;
		if (u -> xtend)
			u -> rds = rds;
		else {
			u -> rds = False;
			dat_clear (& u -> delay);
			dat_clear (& u -> expire);
		}
		if (stout >= 0)
			u -> send_tout = stout;
		if (retry >= 0)
			u -> send_retry = retry;
		if (rtout >= 0)
			u -> rds_tout = rtout;
		if (u -> xtend) {
			if (delay)
				u -> delay = *delay;
			if (expire)
				u -> expire = *expire;
		}
	}
}

void
ucp_set_convtable (void *up, void *ctab)
{
	ucp	*u = (ucp *) up;
	
	MCHK (u);
	if (u) {
		if (u -> ctab)
			cv_free (u -> ctab);
		u -> ctab = ctab;
	}
}

void
ucp_add_convtable (void *up, void *ctab)
{
	ucp	*u = (ucp *) up;
	
	MCHK (u);
	if (u) {
		if (! u -> ctab)
			u -> ctab = cv_new ();
		if (u -> ctab)
			cv_merge (u -> ctab, ctab, True);
	}
}
/*}}}*/
/*{{{	new/free/etc */
void *
ucp_new (void *sp)
{
	ucp	*u;
	
	if (u = (ucp *) malloc (sizeof (ucp))) {
# ifndef	NDEBUG
		u -> magic = MAGIC;
# endif		/* NDEBUG */
		u -> sp = sp;
		u -> ctab = NULL;
		u -> logger = NULL;
		u -> uline = NULL;
		u -> callid = NULL;
		u -> xtend = False;
		u -> send_tout = 60;
		u -> send_retry = 3;
		u -> rds_tout = 40;
		dat_clear (& u -> delay);
		dat_clear (& u -> expire);
		u -> rds = False;
		u -> cnr = 0;
	}
	return (void *) u;
}

void *
ucp_free (void *up)
{
	ucp	*u = (ucp *) up;
	
	MCHK (u);
	if (u) {
		if (u -> ctab)
			cv_free (u -> ctab);
		if (u -> uline)
			free (u -> uline);
		if (u -> callid)
			sfree (u -> callid);
		free (u);
	}
	return NULL;
}

int
ucp_preinit (void)
{
	return 0;
}

void
ucp_postdeinit (void)
{
}
/*}}}*/
