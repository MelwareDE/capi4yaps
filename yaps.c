/*	-*- mode: c; mode: fold -*-	*/
/*{{{	includes */
# include	"config.h"
# include	<stdio.h>
# include	<stdlib.h>
# include	<stdarg.h>
# include	<ctype.h>
# include	<unistd.h>
# include	<string.h>
# include	<signal.h>
# include	<time.h>
# include	<setjmp.h>
# include	<sys/types.h>
# include	<sys/stat.h>
# if		HAVE_LOCALE_H
# include	<locale.h>
# endif		/* HAVE_LOCALE_H */
# if		HAVE_GETOPT_H
# include	<getopt.h>
# endif		/* HAVE_GETOPT_H */
# include	"pager.h"
# include	"valid.h"
/*}}}*/
/*{{{	general definitions */
# define	VERSION		"0.96 (alpha software)"

# ifndef	LIBDIR
# define	LIBDIR		NULL
# endif		/* LIBDIR */

# if		HAVE_SIGSETJMP
# undef		jmp_buf
# define	jmp_buf		sigjmp_buf
# undef		setjmp
# define	setjmp(xx)	sigsetjmp ((xx), 1)
# undef		longjmp
# define	longjmp(xx,vv)	siglongjmp ((xx), (vv))
# endif		/* HAVE_SIGSETJMP */

# define	SPLIT_TERM	" ..."
# define	SPLIT_INIT	"... "

# define	OOM		fprintf (stderr, "Out of memory in line %d, aborted\n", __LINE__)
/*}}}*/
/*{{{	configuration file entry names and default values */
# define	CONFIG_SEP	","

# define	SEC_ALIAS	"alias"

# define	CFG_SERVICES	"services"
# define	CFG_CALLID	"call-id"
# define	CFG_SIG		"signature"
# define	CFG_USESIG	"use-signature"
# define	CFG_VERBOSE	"verbose"
# define	CFG_LOGFILE	"logfile"
# define	CFG_LOGSTR	"logstring"
# define	CFG_MODEMS	"modems"
# define	CFG_FREPORT	"final-report"
# define	CFG_SPEED	"speed"
# define	CFG_BPB		"bits-per-byte"
# define	CFG_PARITY	"parity"
# define	CFG_SB		"stopbits"
# define	CFG_PHONE	"phone"
# define	CFG_PROTOCOL	"protocol"
# define	CFG_MAXSIZE	"max-size"
# define	CFG_MAYSPLIT	"may-split"
# define	CFG_MAXMSGS	"max-messages"
# define	CFG_TRUNCATE	"truncate"
# define	CFG_USECID	"use-call-id"
# define	CFG_RMCID	"rm-invalids-cid,remove-invalid-character-call-id"
# define	CFG_RMPID	"rm-invalids-pid,remove-invalid-character-pager-id"
# define	CFG_INSPID	"insert-pager-id"
# define	CFG_CHPID	"change-pid,change-pager-id"
# define	CFG_VALPID	"valid-pid,valid-pager-id"
# define	CFG_CHCID	"change-cid,change-call-id"
# define	CFG_VALCID	"valid-cid,valid-call-id"
# define	CFG_CONVTAB	"conv-table"
# define	CFG_CONVERT	"convert"
# define	CFG_COST	"cost"
# define	CFG_FORCE	"force"
# define	CFG_CANDELAY	"can-delay"
# define	CFG_CANEXP	"can-expire"
# define	CFG_CANRDS	"can-rds,can-request-delivery-status"
# define	CFG_DORDS	"rds,request-delivery-status"
# define	CFG_CHKCID	"check-call-id"
# define	CFG_CHKPID	"check-pager-id"
# define	CFG_DEVICE	"device"
# define	CFG_LCKPRE	"lock-prefix"
# define	CFG_LCKMETHOD	"lock-method"
# define	CFG_INIT	"init"
# define	CFG_LINIT	"local-init"
# define	CFG_DIAL	"dial"
# define	CFG_TIMEOUT	"timeout"
# define	CFG_RESET	"reset"

# define	DEF_SPEED	38400
# define	DEF_BPB		8
# define	DEF_PARITY	"n"
# define	DEF_SB		1
# define	DEF_USECID	False
# define	DEF_INSPID	False
# define	DEF_CONVTAB	NULL
# define	DEF_CONVERT	NULL
# define	DEF_DEVICE	"/dev/modem"
# define	DEF_LCKPRE	"/usr/spool/uucp/LCK.."
# define	DEF_LCKMETHOD	NULL
# define	DEF_INIT	"\\r !d ATZ\\r <OK ATE0V1Q0\\r <OK"
# define	DEF_LINIT	NULL
# define	DEF_DIAL	"ATD%L\\r <60CONNECT|BUSY|NO\\sCARRIER|NO\\sDIALTONE"
# define	DEF_TIMEOUT	10
# define	DEF_RESET	"ATZ\\r <OK"
/*{{{{	ASCII protocol */
/*
 * ASCII protocol specific
 */
# define	CFG_ATOUT	"asc-timeout"
# define	CFG_ALOGIN	"asc-login"
# define	CFG_ALOGOUT	"asc-logout"
# define	CFG_APID	"asc-pagerid"
# define	CFG_AMSG	"asc-message"	
# define	CFG_ANEXT	"asc-next"
# define	CFG_ASYNC	"asc-sync"

# define	DEF_ATOUT	-1
# define	DEF_ALOGIN	NULL
# define	DEF_ALOGOUT	NULL
# define	DEF_APID	"\\P\\r"
# define	DEF_AMSG	"\\M\\r"	
# define	DEF_ANEXT	NULL
# define	DEF_ASYNC	"!f"
/*}}}*/
/*{{{	Script protocol */
/*
 * Script specific
 */
# define	CFG_STYP	"script-type"
# define	CFG_SNAME	"script-name"
# define	CFG_SLOGIN	"scr-login"
# define	CFG_SLOGOUT	"scr-logout"
# define	CFG_SPID	"scr-pagerid"
# define	CFG_SMSG	"scr-message"
# define	CFG_SNEXT	"scr-next"
# define	CFG_SSYNC	"scr-sync"

# define	DEF_STYP	NULL
# define	DEF_SNAME	NULL
# define	DEF_SLOGIN	"login"
# define	DEF_SLOGOUT	"logout"
# define	DEF_SPID	"pagerid"
# define	DEF_SMSG	"message"
# define	DEF_SNEXT	"next"
# define	DEF_SSYNC	"sync"
/*}}}*/
/*{{{	TAP */
/*
 * TAP specific
 */
# define	CFG_TOLD	"tap-old"
# define	CFG_T1		"tap-t1,tap-init-timeout"
# define	CFG_T2		"tap-t2"
# define	CFG_T3		"tap-t3,tap-timeout"
# define	CFG_T4		"tap-t4"
# define	CFG_T5		"tap-t5"
# define	CFG_N1		"tap-n1,tap-init-count"
# define	CFG_N2		"tap-n2,tap-transmit-count"
# define	CFG_N3		"tap-n3"
# define	CFG_TLICNT	"tap-login-count"
# define	CFG_TLOCNT	"tap-logout-count"

# define	DEF_TOLD	False
# define	DEF_T1		-1
# define	DEF_T2		-1
# define	DEF_T3		-1
# define	DEF_T4		-1
# define	DEF_T5		-1
# define	DEF_N1		-1
# define	DEF_N2		-1
# define	DEF_N3		-1
# define	DEF_TLICNT	-1
# define	DEF_TLOCNT	-1
/*}}}*/
/*{{{	UCP */
/*
 * UCP specific
 */
# define	CFG_USTOUT	"ucp-timeout"
# define	CFG_URETRY	"ucp-retry"
# define	CFG_UXTEND	"ucp-extend"
# define	CFG_URTOUT	"ucp-ds-timeout,ucp-delivery-status-timeout"

# define	DEF_USTOUT	-1
# define	DEF_URETRY	-1
# define	DEF_UXTEND	False
# define	DEF_URTOUT	-1
/*}}}*/
/*}}}*/
/*{{{	typedefs and statics */
typedef enum {
	NotTried,
	CantDial,
	NotSend,
	CantSend
}	Reason;

typedef struct {
	Bool		success;
	Reason		reason;
	char		*rmsg;
}	status;

typedef struct _serv {
	char		*name;
	valid_t		*pchk;
	valid_t		*cchk;
	struct _serv	*next;
}	serv;

typedef struct {
	int		nr;
	char		*callid;
	char		*alias;
	char		*pid;
	char		*service;
	Protocol	prot;
	string_t	*msg;
	serv		*s;
	
	/* Status for each message */
	status		st;
}	message;

static Bool	dojump;
static Bool	jumpdone;
static jmp_buf	env, termenv;
static char	*logfile;
static char	*logstr;
/*}}}*/
/*{{{	signal handler & logging */
static
# if		SIG_VOID_RETURN
void
# endif		/* SIG_VOID_RETURN */
# if		SIG_INT_RETURN
int
# endif		/* SIG_INT_RETURN */
handler (int sig)
{
# if		SYSV_SIGNAL
	signal (sig, handler);
# endif		/* SYSV_SIGNAL */
	switch (sig) {
	case SIGINT:
	case SIGHUP:
		if (dojump) {
			dojump = False;
			longjmp (env, sig);
		}
		break;
	case SIGQUIT:
	case SIGTERM:
		if (! jumpdone) {
			jumpdone = True;
			if (dojump) {
				dojump = False;
				longjmp (env, sig);
			} else
				longjmp (termenv, sig);
		}
		break;
	}
# if		SIG_INT_RETURN
	return 0;
# endif		/* SIG_INT_RETURN */
}

static void
do_log (char typ, char *fmt, ...)
{
	va_list		par;
	FILE		*fp;
	int		omask;
	time_t		tim;
	struct tm	*tt;
	
	va_start (par, fmt);
	omask = umask (003);
	if (logfile && ((! logstr) || strchr (logstr, typ)) && (fp = fopen (logfile, "a"))) {
		time (& tim);
		if (tt = localtime (& tim))
			fprintf (fp, "[%2d.%02d.%04d %02d:%02d:%02d] %c ",
				 tt -> tm_mday, tt -> tm_mon + 1, tt -> tm_year + 1900,
				 tt -> tm_hour, tt -> tm_min, tt -> tm_sec, typ);
		vfprintf (fp, fmt, par);
		fprintf (fp, "\n");
		fclose (fp);
	}
	umask (omask);
	va_end (par);
}
/*}}}*/
/*{{{	dialing */
static char *
mkrealphone (char *phone, char *pagerid)
{
	char	*dst;
	int	siz, n, len;
	char	*ptr;
	
	siz = strlen (phone) + 16;
	if (dst = malloc (siz + 2)) {
		for (n = 0; *phone; )
			if (*phone == '%') {
				++phone;
				if (*phone) {
					ptr = NULL;
					switch (*phone) {
					case 'P':
						ptr = pagerid;
						break;
					}
					if (ptr) {
						len = strlen (ptr);
						if (n + len >= siz) {
							siz = n + len + 64;
							if (! (dst = Realloc (dst, siz + 2)))
								break;
						}
						strcpy (dst + n, ptr);
						n += len;
					}
					++phone;
				}
			} else {
				if (n >= siz) {
					siz += 32;
					if (! (dst = Realloc (dst, siz + 2)))
						break;
				}
				dst[n++] = *phone++;
			}
		if (dst)
			dst[n] = '\0';
	}
	return dst;
}

static void *
do_dial (void *cfg, char *service, char *pagerid, char **modem)
{
	void	*sp;
	char	*tmp, *sav, *ptr;
	char	*t2, *s2, *p2;
	char	*device, *lckpre, *lckmethod;
	int	speed, rspeed, bpb, parity, sb;
	int	tout;
	char	*minit, *linit, *dial, *reset, *phone;
	int	siz, len;
	int	n, m;

	sp = NULL;
	*modem = NULL;
	if ((! (tmp = cfg_get (cfg, service, CFG_MODEMS, NULL))) ||
	    (! (tmp = strdup (tmp))))
		return NULL;
	speed = cfg_iget (cfg, service, CFG_SPEED, DEF_SPEED);
	bpb = cfg_iget (cfg, service, CFG_BPB, DEF_BPB);
	if (ptr = cfg_get (cfg, service, CFG_PARITY, DEF_PARITY))
		parity = *ptr;
	else
		parity = '\0';
	sb = cfg_iget (cfg, service, CFG_SB, DEF_SB);
	dial = NULL;
	reset = NULL;
	tout = 0;
	for (ptr = tmp; *ptr; ) {
		sav = ptr;
		ptr = skipch (ptr, ',');
		device = cfg_get (cfg, sav, CFG_DEVICE, DEF_DEVICE);
		lckpre = cfg_get (cfg, sav, CFG_LCKPRE, DEF_LCKPRE);
		lckmethod = cfg_get (cfg, sav, CFG_LCKMETHOD, DEF_LCKMETHOD);
		minit = cfg_get (cfg, sav, CFG_INIT, DEF_INIT);
		linit = cfg_get (cfg, sav, CFG_LINIT, DEF_LINIT);
		dial = cfg_get (cfg, sav, CFG_DIAL, DEF_DIAL);
		reset = cfg_get (cfg, sav, CFG_RESET, NULL);
		tout = cfg_iget (cfg, sav, CFG_TIMEOUT, DEF_TIMEOUT);
		rspeed = cfg_block_iget (cfg, sav, CFG_SPEED, speed);
		if (device && (t2 = strdup (device))) {
			for (p2 = t2; *p2; ) {
				s2 = p2;
				p2 = skipch (p2, ',');
				V (3, ("Trying to open %s for modem %s\n", s2, sav));
				if (sp = tty_open (s2, lckpre, lckmethod)) {
					if (tty_setup (sp, True, True, rspeed, bpb, sb, parity) != -1) {
						if (! (n = setjmp (env))) {
							dojump = True;
							tty_hangup (sp, 500);
							if (((! minit) || (tty_send_expect (sp, tout, minit, NULL) != -1)) &&
							    ((! linit) || (tty_send_expect (sp, tout, linit, NULL) != -1))) {
								dojump = False;
								V (3, ("Using modem %s at %d bps, %d%c%d over %s\n", sav, rspeed, bpb, parity, sb, s2));
								*modem = strdup (sav);
								break;
							}
						} else {
							V (2, ("\n"));
							if ((n == SIGTERM) || (n == SIGQUIT))
								longjmp (termenv, n);
						}
						dojump = False;
					}
					sp = tty_close (sp);
				}
			}
			free (t2);
		}
		if (sp)
			break;
	}
	free (tmp);
	if (sp && dial && (phone = cfg_get (cfg, service, CFG_PHONE, NULL)) &&
	    (phone = mkrealphone (phone, pagerid))) {
		if (! (n = setjmp (env))) {
			dojump = True;
			for (p2 = phone; *p2; ) {
				s2 = p2;
				p2 = skipch (p2, ',');
				siz = strlen (dial) + strlen (phone) + 64;
				if (tmp = malloc (siz + 2)) {
					for (n = 0, m = 0; dial[n]; ) {
						if (dial[n] == '%') {
							++n;
							if (dial[n]) {
								ptr = NULL;
								switch (dial[n]) {
								case 'L':
									ptr = phone;
									break;
								}
								if (ptr) {
									len = strlen (ptr);
									if (m + len >= siz) {
										siz += len + 64;
										if (! (tmp = Realloc (tmp, siz + 2)))
											break;
									}
									strcpy (tmp + m, ptr);
									m += len;
								}
								++n;
							}
						} else {
							if (m + 1 >= siz) {
								siz += 128;
								if (! (tmp = Realloc (tmp, siz + 2)))
									break;
							}
							switch (dial[n]) {
							case '-':
								tmp[m++] = ',';
								break;
							default:
								tmp[m++] = dial[n];
								break;
							}
							++n;
						}
					}
					if (tmp) {
						tmp[m] = '\0';
						V (3, ("Trying do dial %s\n", s2));
						if (tty_send_expect (sp, tout, tmp, NULL) != -1) {
							V (1, ("Dial successful\n"));
							tty_drain (sp, 1);
							while (*p2)
								++p2;
						} else {
							if (*p2)
								tty_hangup (sp, 500);
							else
								sp = tty_close (sp);
						}
						free (tmp);
					}
				}
			}
		} else {
			V (2, ("\n"));
			if (sp) {
				tty_send (sp, "x\r", 2);
				tty_hangup (sp, 500);
				if (reset)
					tty_send_expect (sp, tout, reset, NULL);
				sp = tty_close (sp);
			}
			if ((n == SIGTERM) || (n == SIGQUIT))
				longjmp (termenv, n);
		}
		dojump = False;
		free (phone);
	}
	return sp;
}
/*}}}*/
/*{{{	send ASCII */
static int
asc_send (void *sp, void *ctab, string_t *callid, message *mg, int mcnt, void *cfg, char *service, date_t *delay, date_t *expire, Bool rds)
{
	int	ret;
	void	*ap;
	int	n;
	int	err, nerr;
	int	atout;
	char	*alin, *alout, *apid, *amsg, *anext, *async;
	char	*tmp;
	
	ret = 0;
	if (ap = asc_new (sp)) {
		atout = cfg_iget (cfg, service, CFG_ATOUT, DEF_ATOUT);
		alin = cfg_get (cfg, service, CFG_ALOGIN, DEF_ALOGIN);
		alout = cfg_get (cfg, service, CFG_ALOGOUT, DEF_ALOGOUT);
		apid = cfg_get (cfg, service, CFG_APID, DEF_APID);
		amsg = cfg_get (cfg, service, CFG_AMSG, DEF_AMSG);
		anext = cfg_get (cfg, service, CFG_ANEXT, DEF_ANEXT);
		async = cfg_get (cfg, service, CFG_ASYNC, DEF_ASYNC);
		asc_config (ap, do_log, atout, alin, alout, apid, amsg, anext, async, delay, expire, rds);
		if (ctab) {
			asc_add_convtable (ap, ctab);
			ctab = cv_free (ctab);
		}
		if ((err = asc_login (ap, callid)) == NO_ERR) {
			for (n = 0; n < mcnt; ++n) {
				tmp = schar (mg[n].msg);
				if (tmp)
					err = asc_transmit (ap, mg[n].pid, tmp);
				else
					err = ERR_FATAL;
				if (err == NO_ERR)
					mg[n].st.success = True;
				else
					mg[n].st.reason = CantSend;
				do_log ((err == NO_ERR ? LGS_SENT : LGF_SENT),
					"ASCII Message %ssent to %s via %s: %s", (err == NO_ERR ? "" : "NOT "),
					(mg[n].alias ? mg[n].alias : (mg[n].pid ? mg[n].pid : "")), service, (tmp ? tmp : ""));
				if (ESTOP (err))
					break;
				if (n + 1 < mcnt) {
					if (err == NO_ERR)
						err = asc_next (ap);
					else
						err = asc_sync (ap);
					if (ESTOP (err))
						break;
					else if (err != NO_ERR)
						if ((nerr = asc_sync (ap)) != NO_ERR) {
							if (nerr < err)
								err = nerr;
							break;
						}
				}
			}
			if (n < mcnt)
				ret = 1;
			if ((err != ERR_ABORT) && (asc_logout (ap) != NO_ERR))
				ret = 1;
		} else
			ret = 1;
		asc_free (ap);
	} else
		ret = 1;
	return ret;
}
/*}}}*/
/*{{{	send script */
static int
scr_send (void *sp, void *ctab, string_t *callid, message *mg, int mcnt, void *cfg, char *service, date_t *delay, date_t *expire, Bool rds)
{
	int	ret;
	void	*s;
	char	*typ;
	char	*name;
	char	*scr;
	char	*slogin, *slogout, *spid, *smsg, *snext, *ssync;
	char	*tmp;
	int	n, err, nerr;
	
	ret = 1;
	typ = cfg_get (cfg, service, CFG_STYP, DEF_STYP);
	if (s = scr_new (sp, typ, LIBDIR)) {
		scr_config (s, do_log, delay, expire, rds);
		name = cfg_get (cfg, service, CFG_SNAME, DEF_SNAME);
		if (name)
			if ((*name == '/') || (*name == '+')) {
				if (*name == '+')
					++name;
				if (scr_load_file (s, name) == NO_ERR)
					ret = 0;
			} else if (scr = cfg_get (cfg, service, name, NULL))
				if (scr_load_string (s, scr) == NO_ERR)
					ret = 0;
		if (! ret) {
			slogin = cfg_get (cfg, service, CFG_SLOGIN, DEF_SLOGIN);
			slogout = cfg_get (cfg, service, CFG_SLOGOUT, DEF_SLOGOUT);
			spid = cfg_get (cfg, service, CFG_SPID, DEF_SPID);
			smsg = cfg_get (cfg, service, CFG_SMSG, DEF_SMSG);
			snext = cfg_get (cfg, service, CFG_SNEXT, DEF_SNEXT);
			ssync = cfg_get (cfg, service, CFG_SSYNC, DEF_SSYNC);
			if (ctab) {
				scr_add_convtable (s, ctab);
				ctab = cv_free (ctab);
			}
			if (scr_execute (s, slogin, schar (callid)) == NO_ERR) {
				err = NO_ERR;
				for (n = 0; n < mcnt; ++n) {
					err = scr_execute (s, spid, mg[n].pid);
					do_log ((err == NO_ERR ? LGS_INF : LGF_INF),
						"SCRIPT pagerid %s %ssent via %s",
						(mg[n].alias ? mg[n].alias : (mg[n].pid ? mg[n].pid : "")),
						(err == NO_ERR ? "" : "NOT "), service);
					if (ESTOP (err))
						break;
					else if (err == NO_ERR) {
						tmp = schar (mg[n].msg);
						if (tmp)
							err = scr_execute (s, smsg, tmp);
						else
							err = ERR_FATAL;
						if (err == NO_ERR)
							mg[n].st.success = True;
						else
							mg[n].st.reason = CantSend;
						do_log ((err == NO_ERR ? LGS_INF : LGF_INF),
							"SCRIPT message %s %ssent via %s", (tmp ? tmp : ""), (err == NO_ERR ? "" : "NOT "), service);
						do_log ((err == NO_ERR ? LGS_SENT : LGF_SENT),
							"SCRIPT Message %ssent to %s via %s: %s", (err == NO_ERR ? "" : "NOT "),
							(mg[n].alias ? mg[n].alias : (mg[n].pid ? mg[n].pid : "")), service, (tmp ? tmp : ""));
						if (ESTOP (err))
							break;
					}
					if (n + 1 < mcnt) {
						if (err == NO_ERR)
							err = scr_execute (s, snext, NULL);
						else
							err = scr_execute (s, ssync, NULL);
						if (ESTOP (err))
							break;
						else if (err != NO_ERR)
							if ((nerr = scr_execute (s, ssync, NULL)) != NO_ERR) {
								if (nerr < err)
									err = nerr;
								break;
							}
					}
				}
				if (n < mcnt)
					ret = 1;
				if ((err != ERR_ABORT) && (scr_execute (s, slogout, NULL) != NO_ERR))
					ret = 1;
			} else
				ret = 1;
		}
		scr_free (s);
	}
	return ret;
}
/*}}}*/
/*{{{	send TAP */
static int
tap_send (void *sp, void *ctab, string_t *callid, message *mg, int mcnt, void *cfg, char *service, date_t *delay, date_t *expire, Bool rds)
{
	void		*tp;
	string_t	*field[4];
	int		fld;
	int		n;
	char		*tmp;
	int		err, nerr;
	
	if (! (tp = tap_new (sp)))
		return 1;
	tap_timeouts (tp,
		      cfg_iget (cfg, service, CFG_T1, DEF_T1),
		      cfg_iget (cfg, service, CFG_T2, DEF_T2),
		      cfg_iget (cfg, service, CFG_T3, DEF_T3),
		      cfg_iget (cfg, service, CFG_T4, DEF_T4),
		      cfg_iget (cfg, service, CFG_T5, DEF_T5));
	tap_retries (tp,
		     cfg_iget (cfg, service, CFG_N1, DEF_N1),
		     cfg_iget (cfg, service, CFG_N2, DEF_N2),
		     cfg_iget (cfg, service, CFG_N3, DEF_N3),
		     cfg_iget (cfg, service, CFG_TLICNT, DEF_TLICNT),
		     cfg_iget (cfg, service, CFG_TLOCNT, DEF_TLOCNT));
	tap_config (tp, do_log, cfg_bget (cfg, service, CFG_TOLD, DEF_TOLD));
	if (ctab) {
		tap_add_convtable (tp, ctab);
		ctab = cv_free (ctab);
	}
	if (tap_login (tp, NULL, '\0', NULL, callid) != NO_ERR) {
		tap_free (tp);
		return 1;
	}
	err = NO_ERR;
	for (n = 0; n < mcnt; ++n) {
		fld = 0;
		field[fld++] = snewc (mg[n].pid);
		field[fld++] = snew (mg[n].msg -> str, mg[n].msg -> len);
		field[fld] = NULL;
		nerr = tap_transmit (tp, field, (n + 1 < mcnt ? False : True));
		if (nerr == NO_ERR)
			mg[n].st.success = True;
		else
			mg[n].st.reason = CantSend;
		tmp = schar (mg[n].msg);
		do_log ((nerr == NO_ERR ? LGS_SENT : LGF_SENT),
			"TAP Message %ssent to %s via %s: %s", (nerr == NO_ERR ? "" : "NOT "),
			(mg[n].alias ? mg[n].alias : (mg[n].pid ? mg[n].pid : "")), service,
			(tmp ? tmp : ""));
		if (nerr != NO_ERR)
			err = nerr;
		for (fld = 0; field[fld]; ++fld)
			sfree (field[fld]);
		if (ESTOP (err))
			break;
	}
	if ((err != ERR_ABORT) && ((n = tap_logout (tp)) != NO_ERR))
		if (err == NO_ERR)
			err = n;
	tap_free (tp);
	return (err != NO_ERR) ? 1 : 0;
}
/*}}}*/
/*{{{	send UCP */
static int
ucp_send (void *sp, void *ctab, string_t *callid, message *mg, int mcnt, void *cfg, char *service, date_t *delay, date_t *expire, Bool rds)
{
	void		*up;
	int		n;
	int		err, nerr;
	string_t	*pid;
	char		*tmp;	

	if (! (up = ucp_new (sp)))
		return 1;
	err = ERR_FATAL;
	if (ctab) {
		ucp_add_convtable (up, ctab);
		ctab = cv_free (ctab);
	}
	ucp_config (up, do_log,
		    cfg_bget (cfg, service, CFG_UXTEND, DEF_UXTEND),
		    cfg_iget (cfg, service, CFG_USTOUT, DEF_USTOUT),
		    cfg_iget (cfg, service, CFG_URETRY, DEF_URETRY),
		    cfg_iget (cfg, service, CFG_URTOUT, DEF_URTOUT),
		    delay, expire, rds);
	if ((err = ucp_login (up, callid)) == NO_ERR) {
		for (n = 0; n < mcnt; ++n) {
			pid = snewc (mg[n].pid);
			if (pid) {
				nerr = ucp_transmit (up, pid, mg[n].msg, (n + 1 < mcnt ? False : True));
				if (nerr == NO_ERR)
					mg[n].st.success = True;
				else
					mg[n].st.reason = CantSend;
			} else
				nerr = ERR_FATAL;
			sfree (pid);
			tmp = schar (mg[n].msg);
			do_log ((nerr == NO_ERR ? LGS_SENT : LGF_SENT),
				"UCP Message %ssent to %s via %s: %s", (nerr == NO_ERR ? "" : "NOT "),
				(mg[n].alias ? mg[n].alias : (mg[n].pid ? mg[n].pid : "")), service,
				(tmp ? tmp : ""));
			if (nerr != NO_ERR)
				err = nerr;
			if (ESTOP (err))
				break;
		}
		if ((err != ERR_ABORT) && ((n = ucp_logout (up)) != NO_ERR))
			if (err == NO_ERR)
				err = n;
	}
	ucp_free (up);
	return (err != NO_ERR) ? 1 : 0;
}
/*}}}*/
/*{{{	support routines */
static Protocol
getproto (char *str)
{
	if (str)
		if (! strcmp (str, "ascii"))
			return Ascii;
		else if (! strcmp (str, "script"))
			return Script;
		else if (! strcmp (str, "tap"))
			return Tap;
		else if (! strcmp (str, "ucp"))
			return Ucp;
	return Unknown;
}

static string_t *
readfile (char *fname)
{
	FILE		*fp;
	string_t	*str;
	int		size;
	struct stat	st;
	
	str = NULL;
	if (fp = fopen (fname, "r")) {
		size = 0;
		if (fstat (fileno (fp), & st) != -1)
			size = st.st_size;
		if ((size > 0) && (str = snew (NULL, size + 1)))
			if (fread (str -> str, sizeof (char), size, fp) == size)
				str -> len = size;
			else
				str = sfree (str);
		fclose (fp);
	}
	return str;
}

static string_t *
read_stdin (void)
{
	string_t	*str;
	int		ch;
	
	if (str = snew (NULL, 256))
		while ((ch = getchar ()) != EOF) {
			if (str -> len >= str -> size)
				sexpand (str, str -> size + 64);
			str -> str[str -> len++] = (char_t) ch;
		}
	return str;
}

static void
extend_convtable (void *ctab, void *cfg, char *service)
{
	char	*str;
	char	*cv, *cvs;
	char	*ptr, *sav;
	char	*src, *dst;
	int	n;
	char	*defconv[] = {
		"no-control",
		"control",
		"no-8bit",
		"8bit",
		"numeric",
		NULL
	};
	
	if ((str = cfg_get (cfg, service, CFG_CONVERT, DEF_CONVERT)) &&
	    (str = strdup (str))) {
		for (cv = str; *cv; ) {
			cvs = cv;
			cv = skipch (cv, ',');
			if (*cvs == '*') {
				++cvs;
				for (n = 0; defconv[n]; ++n)
					if (! strcmp (cvs, defconv[n]))
						break;
				switch (n) {
				case 0:		/* no-control */
					for (n = 0; n < 32; ++n) {
						cv_invalid (ctab, (char_t) n);
						cv_invalid (ctab, (char_t) (n | 0x80));
					}
					cv_invalid (ctab, (char_t) '\x7f');
					break;
				case 1:		/* control */
					for (n = 0; n < 32; ++n) {
						cv_undefine (ctab, (char_t) n);
						cv_undefine (ctab, (char_t) (n | 0x80));
					}
					cv_undefine (ctab, (char_t) '\x7f');
					break;
				case 2:		/* no-8bit */
					for (n = 128; n < 256; ++n)
						cv_invalid (ctab, (char_t) n);
					break;
				case 3:		/* 8bit */
					for (n = 128; n < 256; ++n)
						cv_undefine (ctab, (char_t) n);
					break;
				case 4:		/* numeric */
					for (n = 0; n < 256; ++n)
						if (n && strchr ("0123456789", (char) n))
							cv_undefine (ctab, (char_t) n);
						else
							cv_invalid (ctab, (char_t) n);
					break;
				}
			} else if ((cvs = cfg_get (cfg, service, cvs, NULL)) &&
				   (cvs = strdup (cvs))) {
				for (ptr = cvs; ptr; ) {
					sav = ptr;
					if (ptr = strchr (ptr, '\n'))
						*ptr++ = '\0';
					for (src = sav; isspace (*src); ++src)
						;
					if (*src && (*src != '#')) {
						dst = skip (src);
						if (*src && *dst)
							cv_sdefine (ctab, src, dst);
					}
				}
				free (cvs);
			}
		}
		free (str);
	}
}

static int
check (char *str, char *pat)
{
	Bool	done, incs;
	char	*ptr, *sav, *val;
	int	slen;
	int	n, m, chk;
	char	*p;
	char	*chtab[] = {
		"type",
		"length",
		"minimum",
		"maximum",
		NULL
	},	*tytab[] = {
		"numeric",
		"sedecimal",
		"lower",
		"upper",
		"alpha",
		"alphanumeric",
		"print",
		"ascii",
		NULL
	};

	if (! pat)
		return 0;
	done = False;
	if (*pat == '+') {
		++pat;
		if (pat = strdup (pat)) {
			done = True;
			slen = strlen (str);
			for (ptr = pat; *ptr && done; ) {
				sav = ptr;
				ptr = skipch (ptr, ',');
				val = skipch (sav, '=');
				for (n = 0; chtab[n]; ++n)
					if (! strcmp (sav, chtab[n]))
						break;
				switch (n) {
				case 0:		/* type */
					for (m = 0; tytab[m]; ++m)
						if (! strcmp (val, tytab[m]))
							break;
					for (p = str; *p; ++p) {
						switch (m) {
						default:
							chk = 1;
							break;
						case 0:		/* numeric */
							chk = isdigit (*p);
							break;
						case 1:		/* sedecimal */
							chk = isxdigit (*p);
							break;
						case 2:		/* lower */
							chk = islower (*p);
							break;
						case 3:		/* upper */
							chk = isupper (*p);
							break;
						case 4:		/* alpha */
							chk = isalpha (*p);
							break;
						case 5:		/* alphanumeric */
							chk = isalnum (*p);
							break;
						case 6:		/* print */
							chk = isprint (*p);
							break;
						case 7:		/* ascii */
							chk = isascii (*p);
							break;
						}
						if (! chk)
							return -1;
					}
					break;
				case 1:		/* length */
					if (slen != atoi (val))
						return -1;
					break;
				case 2:		/* minimum */
					if (slen < atoi (val))
						return -1;
					break;
				case 3:		/* maximum */
					if (slen > atoi (val))
						return -1;
					break;
				}
			}
		}
		if (! done)
			return -1;
	} else {
		while (*str && *pat) {
			incs = True;
			switch (tolower (*pat)) {
			case '>':
				done = True;
				incs = False;
				break;
			case '<':
				return -1;
				break;
			case '1':
				if (! isdigit (*str))
					return -1;
				break;
			case 'h':
				if (! isxdigit (*str))
					return -1;
				break;
			case 'l':
				if (! islower (*str))
					return -1;
				break;
			case 'u':
				if (! isupper (*str))
					return -1;
				break;
			case 'a':
				if (! isalpha (*str))
					return -1;
				break;
			case 'n':
				if (! isalnum (*str))
					return 1;
				break;
			case 'p':
				if (! isprint (*str))
					return -1;
				break;
			case 'x':
				if (! isascii (*str))
					return -1;
				break;
			}
			if ((! done) || *(pat + 1))
				++pat;
			if (incs)
				++str;
		}
		if ((! done) && (*str || *pat))
			return -1;
	}
	return 0;
}
/*}}}*/
/*{{{	preparse receiver and messages */
static int
ncompare (const void *a, const void *b)
{
	return ((message *) a) -> nr - ((message *) b) -> nr;
}

static int
mcompare (const void *a, const void *b)
{
	const message	*am = (message *) a,
			*bm = (message *) b;
	int		n;

	n = strcmp (am -> service, bm -> service);
	if (! n)
		n = strcmp (am -> pid, bm -> pid);
	return n;
}

static char *
do_replace (char *str, int start, int len, char *rplc, int rlen)
{
	int	n, m;
	int	slen, diff;

	if (rlen > len) {
		slen = strlen (str);
		diff = rlen - len;
		if (! (str = Realloc (str, slen + diff + 2)))
			return NULL;
		for (n = slen; n >= start + len; --n)
			str[n + diff] = str[n];
		str[slen + diff] = '\0';
	} else if (rlen < len) {
		for (n = start + rlen, m = start + len; str[m]; ++n, ++m)
			str[n] = str[m];
		str[n] = '\0';
	}
	if (rlen > 0)
		memcpy (str + start, rplc, rlen);
	return str;
}

static void
remove_invalids (char *str, char *rmv)
{
	int	m, n;
	char	*ptr;
	
	for (n = 0, m = 0; str[n]; ++n) {
		for (ptr = rmv; *ptr; ++ptr)
			if (*ptr == str[n])
				break;
		if (! *ptr)
			if (n != m)
				str[m++] = str[n];
			else
				++m;
	}
	str[m] = '\0';
}

static message *
create_messages (void *cfg, char *service, serv *sbase, char **argv, int argc, char *mfile, int *mcnt,
		 char *callid, char *sig, Bool trunc, Bool force, date_t *delay, date_t *expire, Bool rds)
{
	char		**recv;
	int		rcnt, rsiz;
	FILE		*fp;
	message		*mg;
	int		cnt, siz;
	Bool		lforce;
	serv		*srun;
	int		n, m, len;
	char		*str, *ptr, *sav, *tstr, *tptr;
	string_t	*fstdin;
	Bool		isalias;
	Bool		doins;
	int		rsize;
	string_t	*tmp;
	char		*rmv;
	char		*rplc;
	int		start, end;
	Bool		ttrunc;
	char		*tsig;
	int		msize;
	Bool		msplit;
	char		*sr;

	rcnt = 0;
	rsiz = 0;
	recv = NULL;
	for (n = 0; n < argc; ++n) {
		if (rcnt + 2 >= rsiz) {
			rsiz += 16;
			if (! (recv = (char **) Realloc (recv, (rsiz + 2) * sizeof (char *)))) {
				OOM;
				return NULL;
			}
		}
		if (! (recv[rcnt] = strdup (argv[n]))) {
			OOM;
			return NULL;
		}
		++rcnt;
	}
	if (mfile) {
		if (! (fp = fopen (mfile, "r"))) {
			fprintf (stderr, "Unable to open message file %s for reading\n", mfile);
			return NULL;
		}
		while (ptr = getline (fp, False)) {
			sav = skip (ptr);
			if (*ptr && *sav) {
				if (rcnt + 2 >= rsiz) {
					rsiz += 16;
					if (! (recv = (char **) Realloc (recv, (rsiz + 2) * sizeof (char *)))) {
						OOM;
						return NULL;
					}
				}
				if ((! (recv[rcnt] = strdup (ptr))) ||
				    (! (recv[rcnt + 1] = strdup (sav)))) {
					OOM;
					return NULL;
				}
				rcnt += 2;
			}
		}
		fclose (fp);
	}
	if (! recv) {
		fprintf (stderr, "No receiver found\n");
		return NULL;
	}
	recv[rcnt] = NULL;

	for (n = 0; n < rcnt; n += 2) {
		ptr = recv[n];
		if (((! isdigit (*ptr)) || (*ptr == ':')) && (*ptr != '\\')) {
			if (*ptr == ':')
				++ptr;
			if (! (str = strdup (ptr))) {
				OOM;
				return NULL;
			}
			m = 0;
			tstr = 0;
			for (ptr = str; *ptr; ) {
				sav = ptr;
				ptr = skipch (ptr, ',');
				if ((! (tptr = cfg_block_get (cfg, SEC_ALIAS, sav, NULL))) ||
				    (! strchr (tptr, ',')))
					tptr = sav;
				len = strlen (tptr);
				if (tstr = Realloc (tstr, m + len + 2)) {
					if (m)
						tstr[m++] = ',';
					strcpy (tstr + m, tptr);
					m += len;
				}
			}
			free (str);
			free (recv[n]);
			if (! (recv[n] = tstr)) {
				OOM;
				return NULL;
			}
		}
	}

	mg = NULL;
	cnt = 0;
	siz = 0;
	fstdin = NULL;
	for (n = 0; n < rcnt; n += 2)
		if (str = recv[n]) {
			for (ptr = str; *ptr; ) {
				if (cnt >= siz) {
					siz += 4;
					if (! (mg = (message *) Realloc (mg, (siz + 1) * sizeof (message)))) {
						OOM;
						return NULL;
					}
				}
				mg[cnt].nr = cnt;
				sav = ptr;
				ptr = skipch (ptr, ',');
				if (*sav == '/') {
					++sav;
					isalias = False;
				} else if (*sav == ':') {
					++sav;
					isalias = True;
				} else if (! isdigit (*sav)) {
					rmv = cfg_get (cfg, NULL, CFG_RMPID, NULL);
					if (rmv && *sav && strchr (rmv, *sav))
						isalias = False;
					else
						isalias = True;
				} else
					isalias = False;
				if (isalias) {
					if (! (mg[cnt].alias = strdup (sav))) {
						OOM;
						return NULL;
					}
					if (! (tstr = cfg_block_get (cfg, SEC_ALIAS, sav, NULL))) {
						fprintf (stderr, "No alias %s found\n", sav);
						return NULL;
					} else
						sav = tstr;
				} else
					mg[cnt].alias = NULL;
				mg[cnt].pid = strdup (sav);
				sav = recv[n + 1];
				if (*sav == '+') {
					if (! (mg[cnt].msg = readfile (sav + 1))) {
						fprintf (stderr, "Unable to read %s, aborted\n", sav + 1);
						return NULL;
					}
				} else if ((*sav == '-') && (! *(sav + 1))) {
					if (! fstdin)
						if (! (fstdin = read_stdin ())) {
							fprintf (stderr, "Unable to read from stdin, aborted\n");
							return NULL;
						}
					mg[cnt].msg = snew (fstdin -> str, fstdin -> len);
				} else if ((*sav == '.') && (! *(sav + 1)))
					mg[cnt].msg = snew (NULL, 0);
				else {
					if (*sav == '\\')
						++sav;
					mg[cnt].msg = snewc (sav);
				}
				if (! (mg[cnt].pid && mg[cnt].msg)) {
					OOM;
					return NULL;
				}
				mg[cnt].callid = NULL;
				mg[cnt].service = NULL;
				mg[cnt].prot = Unknown;
				mg[cnt].s = NULL;
				memset (& mg[cnt].st, 0, sizeof (mg[cnt].st));
				rmv = cfg_get (cfg, mg[cnt].service, CFG_RMPID, NULL);
				for (srun = sbase; srun; srun = srun -> next)
					if ((! service) || (! strcmp (srun -> name, service))) {
						if (rmv)
							remove_invalids (mg[cnt].pid, rmv);
						if (v_alidate (srun -> pchk, mg[cnt].pid, & start, & end))
							break;
					}
				if (! srun) {
					fprintf (stderr, "No service for pager id %s found\n", mg[cnt].pid);
					return NULL;
				}
				if (! (mg[cnt].service = strdup (srun -> name))) {
					OOM;
					return NULL;
				}
				if (rplc = cfg_get (cfg, mg[cnt].service, CFG_CHPID, NULL)) {
					if (! strcmp (rplc, "-"))
						rplc = "";
					else if (*rplc == '\\')
						++rplc;
					if (! (mg[cnt].pid = do_replace (mg[cnt].pid, start, end - start, rplc, strlen (rplc)))) {
						OOM;
						return NULL;
					}
				}
				mg[cnt].s = srun;
				V (4, ("Found service %s for %s\n", mg[cnt].service, mg[cnt].pid));
				++cnt;
			}
			free (str);
		}
	free (recv);
	if (fstdin)
		sfree (fstdin);
	if (cnt > 1)
		qsort (mg, cnt, sizeof (message), mcompare);
	for (n = 0; n < cnt; ++n) {
		sr = mg[n].service;
		if ((mg[n].prot = getproto (cfg_get (cfg, sr, CFG_PROTOCOL, NULL))) == Unknown) {
			fprintf (stderr, "Unknown protocol for service %s\n", sr);
			return NULL;
		}
		if (! callid)
			ptr = cfg_get (cfg, sr, CFG_CALLID, NULL);
		else
			ptr = callid;
		if (ptr)
			if (! (mg[n].callid = strdup (ptr))) {
				OOM;
				return NULL;
			} else if (rmv = cfg_get (cfg, sr, CFG_RMCID, NULL))
				remove_invalids (mg[n].callid, rmv);
		if (cfg_bget (cfg, sr, CFG_USECID, DEF_USECID)) {
			if (! mg[n].callid) {
				fprintf (stderr, "Need caller id for service %s\n", sr);
				return NULL;
			}
		} else if (mg[n].callid) {
			free (mg[n].callid);
			mg[n].callid = NULL;
		}
		if (mg[n].callid) {
			if (mg[n].s && mg[n].s -> cchk)
				if (! v_alidate (mg[n].s -> cchk, mg[n].callid, & start, & end)) {
					fprintf (stderr, "Call ID %s is invalid for service %s\n", mg[n].callid, mg[n].service);
					return NULL;
				}
				if (rplc = cfg_get (cfg, mg[n].service, CFG_CHCID, NULL)) {
					if (! strcmp (rplc, "-"))
						rplc = "";
					else if (*rplc == '\\')
						++rplc;
					if (! (mg[n].callid = do_replace (mg[n].callid, start, end - start, rplc, strlen (rplc)))) {
						OOM;
						return NULL;
					}
				}
			if (check (mg[n].callid, cfg_get (cfg, sr, CFG_CHKCID, NULL)) < 0) {
				fprintf (stderr, "Invalid caller id %s for service %s\n", mg[n].callid, sr);
				return NULL;
			}
		}
		if (check (mg[n].pid, cfg_get (cfg, sr, CFG_CHKPID, NULL)) < 0) {
			fprintf (stderr, "Invalid pager id %s for service %s\n", mg[n].pid, sr);
			return NULL;
		}
		if (force)
			lforce = force;
		else
			lforce = cfg_bget (cfg, sr, CFG_FORCE, False);
		if (delay && (! cfg_bget (cfg, sr, CFG_CANDELAY, False)) && (! lforce)) {
			fprintf (stderr, "Service %s is unable to delay a message\n", sr);
			return NULL;
		}
		if (expire && (! cfg_bget (cfg, sr, CFG_CANEXP, False)) && (! lforce)) {
			fprintf (stderr, "Service %s is unable to set expiration\n", sr);
			return NULL;
		}
		if (! rds)
			rds = cfg_bget (cfg, sr, CFG_DORDS, False);
		if (rds && (! cfg_bget (cfg, sr, CFG_CANRDS, False)) && (! lforce)) {
			fprintf (stderr, "Service %s is unable to report delivery status\n", sr);
			return NULL;
		}
		if (! sig)
			tsig = cfg_get (cfg, sr, CFG_SIG, NULL);
		else
			tsig = sig;
		if (tsig && (! cfg_bget (cfg, sr, CFG_USESIG, False)))
			tsig = NULL;
		if (tsig && *tsig)
			if (! (scatc (mg[n].msg, " ") && scatc (mg[n].msg, tsig))) {
				OOM;
				return NULL;
			}
		mg[n].s = NULL;
	}
	for (n = 0; n < cnt; ++n) {
		sr = mg[n].service;
		if (! trunc)
			ttrunc = cfg_bget (cfg, sr, CFG_TRUNCATE, False);
		else
			ttrunc = trunc;
		msize = cfg_iget (cfg, sr, CFG_MAXSIZE, 0);
		msplit = cfg_bget (cfg, sr, CFG_MAYSPLIT, False);
		if ((msize > 0) && (mg[n].msg -> len > msize))
			if (trunc)
				mg[n].msg -> len = msize;
			else if (! msplit) {
				fprintf (stderr, "Unable to split message for service %s\n", sr);
				return NULL;
			} else {
				doins = (msize > 40) ? True : False;
				if (cnt >= siz) {
					siz += 4;
					if (! (mg = (message *) Realloc (mg, (siz + 1) * sizeof (message)))) {
						OOM;
						return NULL;
					}
				}
				for (m = cnt; m > n + 1; --m)
					mg[m] = mg[m - 1];
				mg[n + 1].nr = mg[n].nr;
				mg[n + 1].callid = mg[n].callid ? strdup (mg[n].callid) : NULL;
				mg[n + 1].alias = mg[n].alias ? strdup (mg[n].alias) : NULL;
				mg[n + 1].pid = strdup (mg[n].pid);
				mg[n + 1].service = strdup (mg[n].service);
				mg[n + 1].prot = mg[n].prot;
				if (doins)
					rsize = msize - (sizeof (SPLIT_INIT) - 1);
				else
					rsize = msize;
				for (m = 1; (m < 15) && (m < rsize); ++m)
					if (sisspace (mg[n].msg, rsize - m)) {
						rsize -= m;
						sdel (mg[n].msg, rsize, 1);
						break;
					}
				if (doins) {
					if (mg[n + 1].msg = snewc (SPLIT_INIT))
						if (tmp = scut (mg[n].msg, rsize, mg[n].msg -> len - rsize)) {
							if (! scat (mg[n + 1].msg, tmp))
								mg[n + 1].msg = sfree (mg[n + 1].msg);
							else {
								mg[n].msg -> len = rsize;
								if (! scatc (mg[n].msg, SPLIT_TERM))
									mg[n].msg = sfree (mg[n].msg);
							}
							sfree (tmp);
						} else
							mg[n + 1].msg = sfree (mg[n + 1].msg);
				} else {
					mg[n + 1].msg = scut (mg[n].msg, rsize, mg[n].msg -> len - rsize);
					mg[n].msg -> len = rsize;
				}
				mg[n + 1].s = NULL;
				memset (& mg[n + 1].st, 0, sizeof (mg[n + 1].st));
				srelease (mg[n].msg);
				if (! (mg[n + 1].pid && mg[n + 1].msg && mg[n + 1].service && mg[n].msg)) {
					OOM;
					return NULL;
				}
				++cnt;
			}
	}
# ifndef	NDEBUG	
	if (verbose >= 4) {
		(*verbout) ("Sending following message%s:\n", (cnt == 1 ? "" : "s"));
		for (n = 0; n < cnt; ++n)
			if (mg[n].pid && mg[n].msg)
				(*verbout) ("%-12s (%s, %s): %s\n", (mg[n].alias ? mg[n].alias : mg[n].pid), mg[n].service, mg[n].pid, schar (mg[n].msg));
	}
# endif		/* NDEBUG */
	*mcnt = cnt;
	return mg;
}
/*}}}*/
/*{{{	calculate costs */
# define	WDAY(cc1,cc2)		((((unsigned char) (cc1)) << 8) | ((unsigned char) (cc2)))
# define	DAY			(60 * 24)

typedef struct _ttable {
	int	wday;
	int	start, end;
	double	elen;
	struct _ttable
		*next;
}	ttable;

static char	*vtab[] = {
	"fixed",
	"entity-length",
	"max-entities",
	"dial-overhead",
	"cost",
	"unit",
	"timetable",
	"remainder",
	NULL
};

static int
tonum (char ch)
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
	}
}

static ttable *
parse_timetable (char *str)
{
	ttable	*base, *prev, *tmp;
	char	*ptr, *sav, *cptr;
	int	n;
	
	base = NULL;
	prev = NULL;
	for (ptr = str; *ptr; ) {
		sav = ptr;
		ptr = skipch (ptr, ';');
		cptr = skipch (sav, '=');
		if (tmp = (ttable *) malloc (sizeof (ttable))) {
			tmp -> wday = 0;
			tmp -> start = 0;
			tmp -> end = DAY - 1;
			tmp -> elen = atof (cptr);
			tmp -> next = NULL;
			while (*sav)
				if (isalpha (*sav) && isalpha (*(sav + 1))) {
					*sav = tolower (*sav);
					*(sav + 1) = tolower (*(sav + 1));
					switch (WDAY (*sav, *(sav + 1))) {
					case WDAY ('s', 'o'):	tmp -> wday |= (1 << 0);	break;
					case WDAY ('m', 'o'):	tmp -> wday |= (1 << 1);	break;
					case WDAY ('t', 'u'):	tmp -> wday |= (1 << 2);	break;
					case WDAY ('w', 'e'):	tmp -> wday |= (1 << 3);	break;
					case WDAY ('t', 'h'):	tmp -> wday |= (1 << 4);	break;
					case WDAY ('f', 'r'):	tmp -> wday |= (1 << 5);	break;
					case WDAY ('s', 'a'):	tmp -> wday |= (1 << 6);	break;
					case WDAY ('w', 'k'):
						tmp -> wday |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);
						break;
					case WDAY ('s', 's'):
						tmp -> wday |= (1 << 0) | (1 << 6);
						break;
					case WDAY ('a', 'l'):
						tmp -> wday |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
						break;
					}
					sav += 2;
				} else
					break;
			while (*sav && (! isdigit (*sav)))
				if (*sav == '-')
					break;
			if (isdigit (*sav)) {
				n = 0;
				while (isdigit (*sav)) {
					n *= 10;
					n += tonum (*sav++);
				}
				tmp -> start = (n / 100) * 60 + n % 100;
			}
			while (*sav && (! isdigit (*sav)))
				++sav;
			if (isdigit (*sav)) {
				n = 0;
				while (isdigit (*sav)) {
					n *= 10;
					n += tonum (*sav++);
				}
				tmp -> end = (n / 100) * 60 + n % 100;
			}
			if (prev)
				prev -> next = tmp;
			else
				base = tmp;
			prev = tmp;
		}
	}
	return base;
}

static inline Bool
timetable_valid (ttable *t, int wday, int tim)
{
	if (t -> wday & (1 << wday))
		if (t -> end > t -> start) {
			if ((tim >= t -> start) &&
			    (tim <= t -> end))
				return True;
		} else 
			if ((tim <= t -> end) ||
			    (tim >= t -> start))
				return True;
	return False;
}

static char *
calc_cost (void *cfg, char *service, time_t start, int dur)
{
	char		*ret;
	char		*tstr;
	char		*ptr, *var, *val;
	int		n;
	Bool		fixed;
	double		elen;
	int		ment;
	int		dover;
	double		cost;
	char		*unit;
	char		*ttab;
	int		remainder;
	ttable		*tbase, *trun;
	struct tm	*tt;
	double		fdur;
	int		cur, wday;
	double		sec;
	int		ents;
	double		fcost;

	if ((! (tstr = cfg_get (cfg, service, CFG_COST, NULL))) ||
	    (! (tstr = strdup (tstr))))
		return NULL;
	ret = NULL;
	fixed = False;
	elen = 0.0;
	ment = -1;
	dover = 0;
	cost = 0.0;
	unit = NULL;
	ttab = NULL;
	remainder = 2;
	for (ptr = tstr; *ptr; ) {
		var = ptr;
		ptr = skipch (ptr, ',');
		val = skipch (var, '=');
		for (n = 0; vtab[n]; ++n)
			if (! strcmp (vtab[n], var))
				break;
		switch (n) {
		case 0:		/* fixed */
			fixed = True;
			break;
		case 1:		/* entity-length */
			elen = atof (val);
			break;
		case 2:		/* max-entities */
			ment = atoi (val);
			break;
		case 3:		/* dial-overhead */
			dover = atoi (val);
			break;
		case 4:		/* cost */
			cost = atof (val);
			break;
		case 5:		/* unit */
			unit = val;
			break;
		case 6:		/* timetable */
			ttab = val;
			break;
		case 7:		/* remainder */
			remainder = atoi (val);
			break;
		}
	}
	if (dover > 0) {
		start += dover;
		dur -= dover;
		if (dur < 1)
			dur = 1;
	}
	if (ttab && (tbase = parse_timetable (ttab))) {
		if (tt = localtime (& start)) {
			cur = tt -> tm_hour * 60 + tt -> tm_min;
			sec = (double) tt -> tm_sec;
			wday = tt -> tm_wday;
			ents = 0;
			fdur = 0;
			trun = NULL;
			while (fdur <= (double) dur) {
				if ((! trun) || (! timetable_valid (trun, wday, cur))) {
					for (trun = tbase; trun; trun = trun -> next)
						if (timetable_valid (trun, wday, cur))
							break;
					if (! trun)
						trun = tbase;
				}
				if (fixed) {
					cost = trun -> elen;
					break;
				}
				if (trun -> elen < 0.1)
					break;
				++ents;
				fdur += trun -> elen;
				sec += trun -> elen;
				while (sec >= 60.0) {
					sec -= 60.0;
					++cur;
					while (cur >= DAY) {
						cur -= DAY;
						wday++;
						if (wday > 6)
							wday = 0;
					}
				}
			}
		}
		while (tbase) {
			trun = tbase;
			tbase = tbase -> next;
			free (trun);
		}
	} else
		ents = (int) ((double) dur / elen) + 1;
	if ((ment > 0) && (ents > ment))
		ents = ment;
	if (fixed)
		fcost = cost;
	else
		fcost = cost * (double) ents;
	if (fcost > 0.01) {
		do_log (LG_COST, "Session costs aprox. %.*f %s", remainder, fcost, (unit ? unit : ""));
		if (ret = malloc (64 + (unit ? strlen (unit) : 0))) {
			sprintf (ret, "%.*f", remainder, fcost);
			if (unit) {
				for (ptr = ret; *ptr; ++ptr)
					;
				*ptr++ = ' ';
				strcpy (ptr, unit);
			}
		}
	}
	free (tstr);
	return ret;
}
/*}}}*/
/*{{{	send pages */
static int
sendit (void *cfg, int cur, message *mg, int mcnt, int *rerr,
	date_t *delay, date_t *expire, Bool rds)
{
	char		*sr;
	Bool		err;
	int		sig;
	int		n, m;
	char		*ptr;
	int		cnt;
	int		mmsgs;
	Bool		inspid;
	char		*convtab;
	void		*ctab;
	message		*send;
	int		count;
	void		*sp;
	char		*modem, *reset;
	int		tout;
	int		(*transmit) (void *, void *, string_t *, message *, int, void *, char *, date_t *, date_t *, Bool);
	time_t		start, end;
	int		diff;
	string_t	*cid;
	
	sr = mg[cur].service;
	mmsgs = cfg_iget (cfg, sr, CFG_MAXMSGS, 0);
	inspid = cfg_bget (cfg, sr, CFG_INSPID, DEF_INSPID);
	for (n = cur, cnt = 0; n < mcnt; ++n, ++cnt) {
		if (n != cur)
			if (strcmp (mg[n].service, sr))
				break;
			else if ((mmsgs > 0) && (cnt == mmsgs))
				break;
			else if (inspid && strcmp (mg[cur].pid, mg[n].pid))
				break;
		mg[n].st.success = False;
		mg[n].st.reason = NotTried;
		mg[n].st.rmsg = NULL;
	}
	ctab = NULL;
	if (ctab = cv_new ()) {
		if (convtab = cfg_get (cfg, sr, CFG_CONVTAB, DEF_CONVTAB))
			cv_read_table (ctab, convtab);
		extend_convtable (ctab, cfg, sr);
	}
	send = mg + cur;
	count = cnt;
	do_log (LG_SSESSION, "Starting up for service %s", sr);
	time (& start);
	err = 0;
	if (sp = do_dial (cfg, sr, (inspid ? mg[0].pid : NULL), & modem)) {
		for (n = 0; n < cnt; ++n)
			mg[cur + n].st.reason = NotSend;
		if (! (sig = setjmp (env))) {
			dojump = True;
			switch (mg[cur].prot) {
			default:
			case Unknown:
				transmit = NULL;
				break;
			case Ascii:
				transmit = asc_send;
				break;
			case Script:
				transmit = scr_send;
				break;
			case Tap:
				transmit = tap_send;
				break;
			case Ucp:
				transmit = ucp_send;
				break;
			}
			if (transmit) {
				cid = mg[0].callid ? snewc (mg[0].callid) : NULL;
				err = (*transmit) (sp, ctab, cid, send, count, cfg, sr, delay, expire, rds);
				sfree (cid);
			} else
				err = 1;
			tty_drain (sp, 1);
		} else
			V (2, ("\n"));
		dojump = False;
		for (n = 0, m = -1; n < cnt; ++n)
			if (mg[n + cur].st.success == True)
				m = n;
		if (m != -1)
			cnt = m + 1;
		tty_hangup (sp, 500);
		time (& end);
		diff = (int) (end - start);
		if (ptr = calc_cost (cfg, sr, start, diff))
			if (! mg[cur].st.rmsg)
				mg[cur].st.rmsg = ptr;
			else
				free (ptr);
		do_log (LG_ESESSION, "Session %s on service %s (duration %d:%02d)", (err ? "failed" : "succeeded"), sr, diff / 60, diff % 60);
		reset = cfg_get (cfg, modem, CFG_RESET, DEF_RESET);
		if (reset) {
			tout = cfg_iget (cfg, modem, CFG_TIMEOUT, DEF_TIMEOUT);
			tty_send_expect (sp, tout, reset, NULL);
		}
		tty_close (sp);
		if (modem)
			free (modem);
		if ((sig == SIGTERM) || (sig == SIGQUIT))
			longjmp (termenv, sig);
	} else {
		for (n = cur + 1; n < mcnt; ++n)
			if (strcmp (sr, mg[n].service))
				break;
		cnt = n - cur;
		for (n = 0; n < cnt; ++n)
			mg[cur + n].st.reason = CantDial;
		do_log (LG_ESESSION, "Unable to dial service %s", sr);
		fprintf (stderr, "Unable to dial %s\n", sr);
		err = 1;
	}
	if (err)
		*rerr = err;
	return cur + cnt;
}
/*}}}*/
/*{{{	print configuration values */
static void
print_config (void *cfg, char *service, char **vars)
{
	int	n;
	char	*val;
	
	while (*vars) {
		printf ("%s ->", *vars);
		for (n = service ? 0 : 1; n < 2; ++n) {
			if (n && service)
				printf (" (");
			else
				printf (" ");
			if (val = n ? cfg_get (cfg, NULL, *vars, NULL) : cfg_block_get (cfg, service, *vars, NULL))
				printf ("%s", val);
			else
				printf ("*none*");
			if (n && service)
				printf (")");
			if (! n)
				printf (" ->");
			else
				printf ("\n");
		}
		++vars;
	}
}
/*}}}*/
/*{{{	getopt */
# if		! HAVE_GETOPT && ! HAVE_GETOPT_LONG
# if		NEED_OPTIND_OPTARG
# undef		NEED_OPTIND_OPTARG
# endif		/* NEED_OPTIND_OPTARG */
static int	optind = 1;
static int	optidx = 0;
static char	*optarg = NULL;

static int
getopt (int argc, char **argv, char *what)
{
	char	ch, *ptr;
	
	optarg = NULL;
	if (optind >= argc)
		return -1;
	do {
		if (! argv[optind][optidx]) {
			optidx = 0;
			++optind;
			if (optind >= argc)
				return -1;
		}
		if (optidx == 0) 
			if (argv[optind][0] != '-')
				return -1;
			else
				optidx = 1;
	}	while (! optidx);
	if ((optidx == 1) && (argv[optind][optidx] == '-') && (! argv[optind][optidx + 1])) {
		++optind;
		return -1;
	}
	ch = argv[optind][optidx++];
	if (! (ptr = strchr (what, ch))) {
		fprintf (stderr, "%s: Unknown option -%c\n", argv[0] ? argv[0] : "", ch);
		return '?';
	}
	++ptr;
	if (*ptr == ':')
		if (argv[optind][optidx]) {
			optarg = (argv[optind]) + optidx;
			++optind;
			optidx = 0;
		} else {
			++optind;
			if (optind < argc)
				optarg = argv[optind++];
			else {
				fprintf (stderr, "%s: Missiong parameter for option -%c\n", argv[0] ? argv[0] : "", ch);
				ch = ':';
			}
			optidx = 0;
		}
	return ch;
}
# endif		/* ! HAVE_GETOPT && ! HAVE_GETOPT_LONG */
/*}}}*/
/*{{{	usage/version */
static void
usage (char *pgm)
{
# if		HAVE_GETOPT_LONG
# define	PLONG(xxx)		fprintf xxx
# else		/* HAVE_GETOPT_LONG */
# define	PLONG(xxx)		putc ('\t', stderr)
# endif		/* HAVE_GETOPT_LONG */
	fprintf (stderr, "Usgae: %s [<opts>] {<pagerid> <message>}+\n", pgm);
	fprintf (stderr, "Function: sends messages to paging devices\n");
	fprintf (stderr, "Options:\n");
	PLONG ((stderr, "\t--config=<fname>,          "));
	fprintf (stderr, "-C <fname>      uses alternate global configurtion file\n");
	PLONG ((stderr, "\t--service=<service>,       "));
	fprintf (stderr, "-s <service>    use this service, if applicated\n");
	PLONG ((stderr, "\t--truncate,                "));
	fprintf (stderr, "-t              truncates too long messages\n");
	PLONG ((stderr, "\t--call-id=<callid>,        "));
	fprintf (stderr, "-c <callid>     if possible, use this caller id\n");
	PLONG ((stderr, "\t--signature=<signature>,   "));
	fprintf (stderr, "-S <sig>        add this signature to the message\n");
	PLONG ((stderr, "\t--logfile=<fname>,         "));
	fprintf (stderr, "-l <fname>      log activities to a file\n");
	PLONG ((stderr, "\t--logstring=<string>,      ")),
	fprintf (stderr, "-L <string>     log only these activities\n");
	PLONG ((stderr, "\t--force,                   "));
	fprintf (stderr, "-f              force sending, even if -d/-e/-r is not supported\n");
	PLONG ((stderr, "\t--delay=<date>,            "));
	fprintf (stderr, "-d <date>       when to send the message (if supported)\n");
	PLONG ((stderr, "\t--expire=<date>,           "));
	fprintf (stderr, "-e <date>       when should the mesage expire (if supported)\n");
	PLONG ((stderr, "\t--request-delivery-status, "));
	fprintf (stderr, "-r              request delivery report (if supported)\n");
	PLONG ((stderr, "\t--final-report=<fname>,    "));
	fprintf (stderr, "-R <fname>      store the final report to <fname>\n");
	PLONG ((stderr, "\t--message=file=<fname>,    "));
	fprintf (stderr, "-z <fname>      read pager-id/message pairs from <fname>\n");
	PLONG ((stderr, "\t--verbose[=<level>],       "));
	fprintf (stderr, "-v              increase verbosity\n");
	fprintf (stderr, "\t<pagerid>       this is the receiver of the message\n");
	fprintf (stderr, "\t<message>       the message, if it is prefixed by a plus sign (+), then the remaining part\n");
	fprintf (stderr, "\t                is taken as a filename from where the message is read\n");
	fprintf (stderr, "\n");
	PLONG ((stderr, "\t--print-config,            "));
	fprintf (stderr, "-p              print configuration variables/values\n");
	fprintf (stderr, "\t                the arguments are the variable names to be displayed\n");
	PLONG ((stderr, "\t--version,                 "));
	fprintf (stderr, "-V              show version and compile informations\n");
# ifdef		VERSION
	fprintf (stderr, "\nVersion: %s\n", VERSION);
# endif		/* VERSION */
# undef		PLONG	
}

static void
show_version (char *pgm)
{
	printf ("%s: V. %s\n", pgm ? pgm : "yaps", VERSION);
	printf ("Definitions:");
# if		POSIX_SIGNAL
	printf (" POSIX_SIGNAL");
# endif		/* POSIX_SIGNAL */
# if		BSD_SIGNAL
	printf (" BSD_SIGNAL");
# endif		/* BSD_SIGNAL */
# if		SYSV_SIGNAL
	printf (" SYSV_SIGNAL");
# endif		/* SYSV_SIGNAL */
# if		SIG_VOID_RETURN
	printf (" SIG_VOID_RETURN");
# endif		/* SIG_VOID_RETURN */
# if		SIG_INT_RETURN
	printf (" SIG_INT_RETURN");
# endif		/* SIG_INT_RETURN */
# if		HAVE_SYS_SELECT_H
	printf (" HAVE_SYS_SELECT_H");
# endif		/* HAVE_SYS_SELECT_H */
# if		HAVE_LOCALE_H
	printf (" HAVE_LOCALE_H");
# endif		/* HAVE_LOCALE_H */
# if		HAVE_REGEX_H
	printf (" HAVE_REGEX_H");
# endif		/* HAVE_REGEX_H */
# if		HAVE_SYS_SYSMACROS_H
	printf (" HAVE_SYS_SYSMACROS_H");
# endif		/* HAVE_SYS_SYSMACROS_H */
# if		HAVE_SYS_MKDEV_H
	printf (" HAVE_SYS_MKDEV_H");
# endif		/* HAVE_SYS_MKDEV_H */
# if		HAVE_GETOPT_H
	printf (" HAVE_GETOPT_H");
# endif		/* HAVE_GETOPT_H */
# if		HAVE_TZSET
	printf (" HAVE_TZSET");
# endif		/* HAVE_TZSET */
# if		HAVE_FCHMOD
	printf (" HAVE_FCHMOD");
# endif		/* HAVE_FCHMOD */
# if		HAVE_FCHOWN
	printf (" HAVE_FCHOWN");
# endif		/* HAVE_FCHOWN */
# if		HAVE_SIGSETJMP
	printf (" HAVE_SIGSETJMP");
# endif		/* HAVE_SIGSETJMP */
# if		HAVE_MEMCPY
	printf (" HAVE_MEMCPY");
# endif		/* HAVE_MEMCPY */
# if		HAVE_BCOPY
	printf (" HAVE_BCOPY");
# endif		/* HAVE_BCOPY */
# if		HAVE_MEMSET
	printf (" HAVE_MEMSET");
# endif		/* HAVE_MEMSET */
# if		HAVE_BZERO
	printf (" HAVE_BZERO");
# endif		/* HAVE_BZERO */
# if		HAVE_GETOPT
	printf (" HAVE_GETOPT");
# endif		/* HAVE_GETOPT */
# if		HAVE_GETOPT_LONG
	printf (" HAVE_GETOPT_LONG");
# endif		/* HAVE_GETOPT_LONG */
# if		NEED_OPTIND_OPTARG
	printf (" NEED_OPTIND_OPTARG");
# endif		/* NEED_OPTIND_OPTARG */
# if		BROKEN_REALLOC
	printf (" BROKEN_REALLOC");
# endif		/* BROKEN_REALLOC */
	printf ("\n");
}
/*}}}*/
/*{{{	main */
# if		HAVE_GETOPT_LONG
static struct option	opttab[] = {
	{	"config",	required_argument,	NULL,	'C'	},
	{	"service",	required_argument,	NULL,	's'	},
	{	"truncate",	no_argument,		NULL,	't'	},
	{	"call-id",	required_argument,	NULL,	'c'	},
	{	"signature",	required_argument,	NULL,	'S'	},
	{	"logfile",	required_argument,	NULL,	'l'	},
	{	"logstring",	required_argument,	NULL,	'L'	},
	{	"force",	no_argument,		NULL,	'f'	},
	{	"delay",	required_argument,	NULL,	'd'	},
	{	"expire",	required_argument,	NULL,	'e'	},
	{	"request-delivery-status", no_argument,	NULL,	'r'	},
	{	"final-report",	required_argument,	NULL,	'R'	},
	{	"message-file",	required_argument,	NULL,	'z'	},
	{	"verbose",	optional_argument,	NULL,	'v'	},
	{	"print-config",	no_argument,		NULL,	'p'	},
	{	"version",	no_argument,		NULL,	'V'	},
	{	"help",		no_argument,		NULL,	'h'	},
	{	NULL,		0,			NULL,	0	}
};
# define	GETOPT(what)		getopt_long (argc, argv, (what), opttab, NULL)
# else		/* HAVE_GETOPT_LONG */
# define	GETOPT(what)		getopt (argc, argv, (what))
# endif		/* HAVE_GETOPT_LONG */

int
main (int argc, char **argv)
{
# if		NEED_OPTIND_OPTARG
	extern int	optind;
	extern char	*optarg;
# endif		/* NEED_OPTIND_OPTARG */
	char		*cfgfile;
	void		*cfg;
	char		*home, *lfn;
	int		n, m;
	int		ret;
	char		*svcs;
	serv		*sbase, *sprev, *stmp;
	char		*chk;
	char		*service;
	Bool		trunc;
	char		*callid;
	char		*sig;
	Bool		force;
	date_t		*delay, *expire;
	Bool		rds;
	char		*freport;
	FILE		*fp;
	char		*mfile;
	Bool		pcfg;
	date_t		*cur;
	message		*mg;
	int		mcnt;
	int		onr;
	Bool		succ;
	char		*ptr, *sav;
# if		POSIX_SIGNAL
	struct sigaction
			sact, sign;
# endif		/* POSIX_SIGNAL */

	cfgfile = CFGFILE;
	service = NULL;
	trunc = False;
	callid = NULL;
	sig = NULL;
	force = False;
	delay = NULL;
	expire = NULL;
	rds = False;
	freport = NULL;
	mfile = NULL;
	pcfg = False;
	logfile = NULL;
	logstr = NULL;
	verbose = -1;
# if		HAVE_TZSET	
	tzset ();
# endif		/* HAVE_TZSET */
# if		HAVE_LOCALE_H
	setlocale (LC_ALL, "");
# endif		/* HAVE_LOCALE_H */
	while ((n = GETOPT ("C:s:tc:S:l:L:fd:e:rR:z:vpVh")) != -1)
		switch (n) {
		case 'C':
			cfgfile = optarg;
			break;
		case 's':
			service = optarg;
			break;
		case 't':
			trunc = True;
			break;
		case 'c':
			callid = optarg;
			break;
		case 'S':
			sig = optarg;
			break;
		case 'l':
			logfile = optarg;
			break;
		case 'L':
			logstr = optarg;
			break;
		case 'f':
			force = True;
			break;
		case 'd':
			if (delay)
				delay = dat_free (delay);
			if (! (delay = dat_parse (optarg))) {
				fprintf (stderr, "Invalid date format for delay\n");
				return 1;
			}
			break;
		case 'e':
			if (expire)
				expire = dat_free (expire);
			if (! (expire = dat_parse (optarg))) {
				fprintf (stderr, "Invalid date format for expire\n");
				return 1;
			}
			break;
		case 'r':
			rds = True;
			break;
		case 'R':
			freport = optarg;
			break;
		case 'z':
			mfile = optarg;
			break;
		case 'v':
# if		HAVE_GETOPT_LONG
			if (optarg)
				verbose = atoi (optarg);
			else
# endif		/* HAVE_GETOPT_LONG */
			if (verbose < 0)
				verbose = 1;
			else
				++verbose;
			break;
		case 'p':
			pcfg = True;
			break;
		case 'V':
			show_version (argv[0]);
			return 0;
		case 'h':
		default:
			usage (argv[0]);
			return n == 'h' ? 0 : 1;
		}
	if (delay || expire) {
		if (cur = dat_parse (NULL)) {
			if (delay && (dat_diff (cur, delay) < 0)) {
				fprintf (stderr, "Delay is before now, invalid\n");
				return 1;
			}
			if (expire && (dat_diff (cur, expire) < 0)) {
				fprintf (stderr, "Expiration already reached\n");
				return 1;
			}
			dat_free (cur);
		}
		if (delay && expire)
			if (dat_diff (delay, expire) < 0) {
				fprintf (stderr, "Message expires before it should be send\n");
				return 1;
			}
	}
	cfg = cfg_read (cfgfile, NULL, CONFIG_SEP);
	if ((home = getenv ("HOME")) && (lfn = malloc (strlen (home) + sizeof (LCFGFILE) + 4))) {
		sprintf (lfn, "%s/%s", home, LCFGFILE);
		cfg = cfg_read (lfn, cfg, CONFIG_SEP);
		free (lfn);
	}
	if (pcfg) {
		print_config (cfg, service, argv + optind);
		return 0;
	}
	if (((optind == argc) && (! mfile)) || ((argc - optind) & 1)) {
		if ((optind == argc) && (! mfile))
			fprintf (stderr, "No message\n");
		if ((argc - optind) & 1)
			fprintf (stderr, "Unbalanced pagerid/message\n");
		usage (argv[0]);
		return 1;
	}
	if (mfile && (access (mfile, R_OK) < 0)) {
		fprintf (stderr, "Unable to read message file %s\n", mfile);
		return 1;
	}
	if (verbose < 0)
		verbose = cfg_iget (cfg, NULL, CFG_VERBOSE, 0);
	if (! logfile)
		logfile = cfg_get (cfg, NULL, CFG_LOGFILE, NULL);
	if (! logstr)
		logstr = cfg_get (cfg, NULL, CFG_LOGSTR, NULL);
	if (! (svcs = cfg_get (cfg, NULL, CFG_SERVICES, NULL))) {
		fprintf (stderr, "No services definied\n");
		return 1;
	}
	if (! (svcs = strdup (svcs))) {
		OOM;
		return 1;
	}
	sbase = NULL;
	sprev = NULL;
	for (ptr = svcs; *ptr; ) {
		sav = ptr;
		ptr = skipch (ptr, ',');
		if (! (chk = cfg_block_get (cfg, sav, CFG_VALPID, NULL))) {
			fprintf (stderr, "Service %s has no pattern for matching pager ids\n", sav);
			return 1;
		}
		if ((stmp = (serv *) malloc (sizeof (serv))) &&
		    (stmp -> name = strdup (sav))) {
			if (! (stmp -> pchk = v_new (chk))) {
				fprintf (stderr, "Pattern %s in service %s is not valid\n", chk, sav);
				return 1;
			}
			stmp -> cchk = NULL;
			if (chk = cfg_get (cfg, sav, CFG_VALCID, NULL))
				if (! (stmp -> cchk = v_new (chk))) {
					fprintf (stderr, "Pattern %s in service %s not valid\n", chk, sav);
					return 1;
				}
			stmp -> next = NULL;
			if (sprev)
				sprev -> next = stmp;
			else
				sbase = stmp;
			sprev = stmp;
		} else {
			OOM;
			return 1;
		}
	}
	if (! sbase) {
		fprintf (stderr, "No services found\n");
		return 1;
	}
	if (! (mg = create_messages (cfg, service, sbase, argv + optind, argc - optind, mfile, & mcnt, callid, sig, trunc, force, delay, expire, rds)))
		return 1;
	while (sbase) {
		stmp = sbase;
		sbase = sbase -> next;
		free (stmp -> name);
		v_free (stmp -> pchk);
		if (stmp -> cchk)
			v_free (stmp -> cchk);
		free (stmp);
	}
	jumpdone = False;
	if (! (ret = setjmp (termenv))) {
		dojump = False;
# if		POSIX_SIGNAL
		sact.sa_handler = handler;
		sign.sa_handler = SIG_IGN;
		sigemptyset (& sact.sa_mask);
		sigemptyset (& sign.sa_mask);
		sigaddset (& sact.sa_mask, SIGHUP);
		sigaddset (& sact.sa_mask, SIGINT);
		sigaddset (& sact.sa_mask, SIGQUIT);
		sigaddset (& sact.sa_mask, SIGTERM);
		sigaddset (& sact.sa_mask, SIGPIPE);
		sigaddset (& sign.sa_mask, SIGHUP);
		sigaddset (& sign.sa_mask, SIGINT);
		sigaddset (& sign.sa_mask, SIGQUIT);
		sigaddset (& sign.sa_mask, SIGTERM);
		sigaddset (& sign.sa_mask, SIGPIPE);
		sact.sa_flags = 0;
		sign.sa_flags = 0;
		sigaction (SIGHUP, & sact, NULL);
		sigaction (SIGINT, & sact, NULL);
		sigaction (SIGQUIT, & sact, NULL);
		sigaction (SIGTERM, & sact, NULL);
		sigaction (SIGPIPE, & sign, NULL);
# else		/* POSIX_SIGNAL */
		signal (SIGHUP, handler);
		signal (SIGINT, handler);
		signal (SIGQUIT, handler);
		signal (SIGTERM, handler);
# ifdef		SIGPIPE	
		signal (SIGPIPE, SIG_IGN);
# endif		/* SIGPIPE */
# endif		/* POSIX_SIGNAL */
		for (n = 0; n < mcnt; )
			n = sendit (cfg, n, mg, mcnt, & ret, delay, expire, rds);
	}
	if (mg) {
		if (! freport)
			freport = cfg_get (cfg, NULL, CFG_FREPORT, NULL);
		fp = NULL;
		if (freport)
			if (! strcmp (freport, "-"))
				fp = stdout;
			else if (! (fp = fopen (freport, "w"))) {
				fprintf (stderr, "Unable to open %s for final report, sending it to stdout\n", freport);
				fp = stdout;
			}
		if (fp && (mcnt > 1))
			qsort (mg, mcnt, sizeof (message), ncompare);
		for (m = 0, onr = -1; m < mcnt; ++m) {
			if (fp && (onr != mg[m].nr)) {
				onr = mg[m].nr;
				succ = True;
				for (n = m; (n < mcnt) && (mg[n].nr == onr); ++n)
					if (! (mg[n].st.success)) {
						succ = False;
						break;
					}
				ptr = NULL;
				if (! succ) {
					switch (mg[n].st.reason) {
					case NotTried:
						ptr = "unable to connect";
						break;
					case CantDial:
						ptr = "could not dial service provider";
						break;
					case NotSend:
						ptr = "unable to send";
						break;
					case CantSend:
						ptr = "could not transmit the message";
						break;
					}
				}
				fprintf (fp, "%c%d %s: %s %s\n", 
					 (succ ? '+' : '-'), mg[m].nr,
					 (mg[m].alias ? mg[m].alias : (mg[m].pid ? mg[m].pid : "")),
					 (mg[m].st.rmsg ? mg[m].st.rmsg : ""),
					 (ptr ? ptr : ""));
			}
			if (mg[m].callid)
				free (mg[m].callid);
			if (mg[m].alias)
				free (mg[m].alias);
			if (mg[m].pid)
				free (mg[m].pid);
			if (mg[m].service)
				free (mg[m].service);
			if (mg[m].msg)
				sfree (mg[m].msg);
			if (mg[m].st.rmsg)
				free (mg[m].st.rmsg);
		}
		free (mg);
		if (fp && (fp != stdout))
			fclose (fp);
	}
	cfg_end (cfg);
	return ret;
}
/*}}}*/
