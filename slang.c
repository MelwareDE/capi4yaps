/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# ifdef		SCRIPT_SLANG
# ifndef	FLOAT_TYPE
# define	FLOAT_TYPE
# endif		/* FLOAT_TYPE */
# include	<stdio.h>
# include	<stdlib.h>
# include	<unistd.h>
# include	<string.h>
# include	<slang.h>
# include	"pager.h"
# include	"script.h"

# define	STARTUP		"Startup.sl"

/*{{{	string class */
static char	*ifmt = NULL;
static char	*ffmt = NULL;

static inline char *
make_scratch (unsigned char typ, VOID_STAR p)
{
	char	buf[128];

	buf[0] = '\0';
	switch (typ) {
	case INT_TYPE:
		sprintf (buf, (ifmt ? ifmt : "%d"), *(int *) p);
		break;
	case FLOAT_TYPE:
		sprintf (buf, (ffmt ? ffmt : "%f"), (double) (*(float64 *) p));
		break;
	}
	return strdup (buf);
}

static int
binop_string (int op, unsigned char atyp, unsigned char btyp,
	      VOID_STAR ap, VOID_STAR bp)
{
	char	*scratch;
	char	*a, *b;
	int	la, lb;
	int	len;
	int	n;
	char	*ptr;
	char	*res;

	scratch = NULL;
	if ((atyp == STRING_TYPE) && (btyp == STRING_TYPE)) {
		a = (char *) ap;
		b = (char *) bp;
	} else if (atyp == STRING_TYPE) {
		a = (char *) ap;
		scratch = make_scratch (btyp, bp);
		b = scratch;
	} else {
		b = (char *) ap;
		scratch = make_scratch (atyp, ap);
		a = scratch;
	}
	if (! (a && b))
		return 0;
	res = NULL;
	switch (op) {
	case SLANG_PLUS:
		la = strlen (a);
		lb = strlen (b);
		if (res = SLMALLOC (la + lb + 1)) {
			strcpy (res, a);
			strcat (res, b);
		}
		break;
	case SLANG_MINUS:
		la = strlen (a);
		lb = strlen (b);
		if (ptr = strstr (a, b)) {
			if (res = SLMALLOC (la - lb + 1)) {
				len = (int) ((unsigned long) ptr - (unsigned long) a);
				strncpy (res, a, len);
				res[len] = '\0';
				strcat (res, a + len + lb);
			}
		} else if (res = SLMALLOC (la + 1))
			strcpy (res, a);
		break;
	case SLANG_TIMES:
		la = strlen (a);
		if ((n = atoi (b)) > 0) {
			if (res = SLMALLOC (la * n + 1)) {
				ptr = res;
				while (n-- > 0) {
					strcpy (ptr, a);
					while (*ptr)
						++ptr;
				}
			}
		} else if (res = SLMALLOC (2))
			res[0] = '\0';
		break;
	case SLANG_DIVIDE:
		for (ptr = a; *ptr; ++ptr)
			if (! strchr (b, *ptr))
				break;
		if (res = SLMALLOC (strlen (ptr) + 1)) {
			strcpy (res, ptr);
			if (res[0]) {
				for (ptr = res; *ptr; ++ptr)
					;
				--ptr;
				while (ptr != res)
					if (strchr (b, *ptr))
						*ptr-- = '\0';
					else
						break;
			}
		}
		break;
	case SLANG_EQ:
		SLang_push_integer (strcmp (a, b) == 0 ? 1 : 0);
		break;
	case SLANG_NE:
		SLang_push_integer (strcmp (a, b) ? 1 : 0);
		break;
	case SLANG_GT:
		SLang_push_integer (strcmp (a, b) > 0 ? 1 : 0);
		break;
	case SLANG_GE:
		SLang_push_integer (strcmp (a, b) >= 0 ? 1 : 0);
		break;
	case SLANG_LT:
		SLang_push_integer (strcmp (a, b) < 0 ? 1 : 0);
		break;
	case SLANG_LE:
		SLang_push_integer (strcmp (a, b) <= 0 ? 1 : 0);
		break;
	default:
		return 0;
	}
	if (scratch)
		free (scratch);
	if (res)
		SLang_push_malloced_string (res);
	return 1;
}

static int
unop_string (int op, unsigned char typ, VOID_STAR p)
{
	char	*s;
	char	*r;
	int	len;
	
	s = (char *) p;
	switch (op) {
	case SLANG_ABS:
		SLang_push_integer (strlen (s));
		break;
	case SLANG_SIGN:
		SLang_push_integer (*s ? 1 : 0);
		break;
	case SLANG_SQR:
		break;
	case SLANG_MUL2:
		len = strlen (s);
		if (r = SLMALLOC (len * 2 + 1)) {
			strcpy (r, s);
			strcat (r, s);
			SLang_push_malloced_string (r);
		}
		break;
	case SLANG_CHS:
		SLang_push_string (s);
		break;
	default:
		return 0;
	}
	return 1;
}

static void
get_format (char **var)
{
	char	*fmt;
	int	fre;

	if (*var) {
		free (*var);
		*var = NULL;
	}
	if (SLang_pop_string (& fmt, & fre))
		return;
	*var = strdup (fmt);
	if (fre)
		SLFREE (fmt);
}

static void
str_iformat (void)
{
	get_format (& ifmt);
}

static void
str_fformat (void)
{
	get_format (& ffmt);
}

static SLang_Name_Type	avail_string[] = {
	/* void		str_iformat (string format);			*/
	MAKE_INTRINSIC (".str_iformat", str_iformat, VOID_TYPE, 0),
	/* void		str_fformat (string format);			*/
	MAKE_INTRINSIC (".str_fformat", str_fformat, VOID_TYPE, 0),
/*
	MAKE_INTRINSIC (".", , _TYPE, 0),
	MAKE_INTRINSIC (".function_name", c_function, TYPE, 0),
	MAKE_VARIABLE (".", & , TYPE, ),
	MAKE_VARIABLE (".var", &c_variable, TYPE, flag),
*/
        SLANG_END_TABLE
};

static int
string_class (void)
{
	if ((! SLang_register_class (STRING_TYPE, NULL, NULL)) ||
	    (! SLang_add_binary_op (STRING_TYPE, STRING_TYPE,
				    (VOID_STAR) binop_string)) ||
	    (! SLang_add_binary_op (STRING_TYPE, INT_TYPE,
				    (VOID_STAR) binop_string)) ||
	    (! SLang_add_binary_op (STRING_TYPE, FLOAT_TYPE,
				    (VOID_STAR) binop_string)) ||
	    (! SLang_add_unary_op (STRING_TYPE,
				   (VOID_STAR) unop_string)) ||
	    (! SLang_add_table (avail_string, "string")))
		return 0;
	return 1;
}
/*}}}*/
/*{{{	statics */
static Bool		isinit = False;
static script		*sls = NULL;
static char		*slline = NULL;
static int		slsiz = 0;
static SLang_Name_Type	*slcb = NULL;
/*}}}*/
/*{{{	callable functions and variables */
static void
sllogger (void)
{
	char	*str;
	int	fre;
	int	typ;
	
	if (SLang_pop_string (& str, & fre) ||
	    SLang_pop_integer (& typ))
		return;
	if (sls && sls -> logger) {
		if (! typ)
			typ = LG_INF;
		(*sls -> logger) ((char) typ, "%s\n", str);
	}
	if (fre)
		SLFREE (str);
}

static void
slcallback (void *sp, string_t *s, char_t sep, void *data)
{
	int	len;
	char	*str;

	if (str = sextract (s)) {
		len = strlen (str);
		if (len + 2 >= slsiz) {
			slsiz = len + 64;
			if (! (slline = Realloc (slline, slsiz + 4)))
				slsiz = 0;
		}
		if (slline) {
			sprintf (slline, "%s%c", str, (char) sep);
			if (slcb) {
				SLang_push_string (slline);
				SLexecute_function (slcb);
			}
		}
		free (str);
	}
}

static void
slsetcb (void)
{
	char	*func, *sep;
	int	fr1, fr2;
	
	if (SLang_pop_string (& sep, & fr2) ||
	    SLang_pop_string (& func, & fr1))
		return;
	if (func && *func)
		slcb = SLang_get_function (func);
	else
		slcb = NULL;
	if (sls && sls -> sp)
		tty_set_line_callback (sls -> sp, slcallback, sep, NULL);
	if (fr1)
		SLFREE (func);
	if (fr2)
		SLFREE (sep);
}

static void
slclrcb (void)
{
	slcb = NULL;
	if (sls && sls -> sp)
		tty_set_line_callback (sls -> sp, NULL, NULL, NULL);
}

static void
slget_line (void)
{
	SLang_push_string (slline ? slline : "");
}

static void
slhangup (void)
{
	int	msec;
	
	if (SLang_pop_integer (& msec))
		return;
	if (sls && sls -> sp)
		tty_hangup (sls -> sp, msec);
}

static int
slsend (void)
{
	int	ret;
	char	*str;
	int	fre;
	
	if (SLang_pop_string (& str, & fre))
		return 0;
	ret = 0;
	if (sls && sls -> sp)
		if (tty_send_string (sls -> sp, str) != -1)
			ret = 1;
	if (fre)
		SLFREE (str);
	return ret;
}

static int
slcsend (void)
{
	int	ret;
	char	*str;
	int	fre;
	char	*rstr;
	
	if (SLang_pop_string (& str, & fre))
		return 0;
	ret = 0;
	if (sls && sls -> sp)
		if (rstr = scr_convert (sls, str)) {
			if (tty_send_string (sls -> sp, rstr) != -1)
				ret = 1;
			free (rstr);
		}
	if (fre)
		SLFREE (str);
	return ret;
}

static int
slexpect (void)
{
	int	ret;
	int	cnt;
	char	**str;
	int	*fre;
	int	*len;
	int	tout;
	int	n;
	
	if (SLang_pop_integer (& cnt))
		return -1;
	ret = -1;
	if ((str = (char **) malloc ((cnt + 2) * sizeof (char *))) &&
	    (fre = (int *) malloc ((cnt + 2) * sizeof (int))) &&
	    (len = (int *) malloc ((cnt + 2) * sizeof (int)))) {
		for (n = cnt - 1; n >= 0; --n)
			if (SLang_pop_string (& str[n], & fre[n]))
				return -1;
			else
				len[n] = strlen (str[n]);
		str[cnt] = NULL;
		len[cnt] = 0;
		if (SLang_pop_integer (& tout))
			return -1;
		if (sls && sls -> sp)
			ret = tty_expect_list (sls -> sp, tout, str, len);
		for (n = 0; n < cnt; ++n)
			if (fre[n])
				SLFREE (str[n]);
		free (str);
		free (fre);
		free (len);
	}
	return ret;
}

static int
slsend_expect (void)
{
	int	tout;
	char	*str;
	int	fre;
	int	ret;
	
	if (SLang_pop_string (& str, & fre) ||
	    SLang_pop_integer (& tout))
		return 0;
	ret = 0;
	if (sls && sls -> sp)
		if (tty_send_expect (sls -> sp, tout, str, NULL) != -1)
			ret = 1;
	if (fre)
		SLFREE (str);
	return ret;
}

static void
sldrain (void)
{
	int	secs;
	
	if (SLang_pop_integer (& secs))
		return;
	if (sls && sls -> sp)
		tty_drain (sls -> sp, secs);
}

static void
slcvdef (void)
{
	int	src, dst;
	
	if (SLang_pop_integer (& dst) ||
	    SLang_pop_integer (& src))
		return;
	if (sls) {
		if (! sls -> ctab)
			sls -> ctab = cv_new ();
		if (sls -> ctab)
			cv_define (sls -> ctab, (char_t) src, (char_t) dst);
	}
}

static void
slcvundef (void)
{
	int	ch;
	
	if (SLang_pop_integer (& ch))
		return;
	if (sls && sls -> ctab)
		cv_undefine (sls -> ctab, (char_t) ch);
}

static void
slcvinval (void)
{
	int	ch;
	
	if (SLang_pop_integer (& ch))
		return;
	if (sls) {
		if (! sls -> ctab)
			sls -> ctab = cv_new ();
		if (sls -> ctab)
			cv_invalid (sls -> ctab, (char_t) ch);
	}
}
static void
slconv (void)
{
	char	*str;
	int	fre;
	char	*rstr;
	
	if (SLang_pop_string (& str, & fre))
		return;
	if (sls)
		rstr = scr_convert (sls, str);
	else
		rstr = NULL;
	SLang_push_string (rstr ? rstr : str);
	if (rstr)
		free (rstr);
	if (fre)
		SLFREE (str);
}

static int	no_err = NO_ERR,
		err_fail = ERR_FAIL,
		err_fatal = ERR_FATAL,
		err_abort = ERR_ABORT;
static date_t	sldelay, slexpire;
static int	slrds = 0;
static int	xFalse = (int) False,
		xTrue = (int) True;
static char	xNull_String[2] = "";
/*}}}*/
/*{{{	function/variable table */
static SLang_Name_Type	avail[] = {
	/* void		logger (string str);				*/
	MAKE_INTRINSIC (".logger", sllogger, VOID_TYPE, 0),
	/* void		setcb (string func, string sep);		*/
	MAKE_INTRINSIC (".setcb", slsetcb, VOID_TYPE, 0),
	/* void		clrcb (void);					*/
	MAKE_INTRINSIC (".clrcb", slclrcb, VOID_TYPE, 0),
	/* string	line (void);					*/
	MAKE_INTRINSIC (".line", slget_line, VOID_TYPE, 0),
	/* void		hangup (int msec);				*/
	MAKE_INTRINSIC (".hangup", slhangup, VOID_TYPE, 0),
	/* int		send (string str);				*/
	MAKE_INTRINSIC (".send", slsend, INT_TYPE, 0),
	/* int		csend (string str);				*/
	MAKE_INTRINSIC (".csend", slcsend, INT_TYPE, 0),
	/* int		expect (int tout, string e1, ..., string en,
				int cnt);				*/
	MAKE_INTRINSIC (".expect", slexpect, INT_TYPE, 0),
	/* int		send_expect (int tout, string str);		*/
	MAKE_INTRINSIC (".send_expect", slsend_expect, INT_TYPE, 0),
	/* void		drain (int secs);				*/
	MAKE_INTRINSIC (".drain", sldrain, INT_TYPE, 0),
	/* void		cvdef (int src, int dst);			*/
	MAKE_INTRINSIC (".cvdef", slcvdef, VOID_TYPE, 0),
	/* void		cvundef (int ch);				*/
	MAKE_INTRINSIC (".cvundef", slcvundef, VOID_TYPE, 0),
	/* void		cvinval (int ch);				*/
	MAKE_INTRINSIC (".cvinval", slcvinval, VOID_TYPE, 0),
	/* string	conv (string str);				*/
	MAKE_INTRINSIC (".conv", slconv, VOID_TYPE, 0),

	MAKE_VARIABLE (".NO_ERR", & no_err, INT_TYPE, 1),
	MAKE_VARIABLE (".ERR_FAIL", & err_fail, INT_TYPE, 1),
	MAKE_VARIABLE (".ERR_FATAL", & err_fatal, INT_TYPE, 1),
	MAKE_VARIABLE (".ERR_ABORT", & err_abort, INT_TYPE, 1),
	MAKE_VARIABLE (".delay_day", & sldelay.day, INT_TYPE, 1),
	MAKE_VARIABLE (".delay_mon", & sldelay.mon, INT_TYPE, 1),
	MAKE_VARIABLE (".delay_year", & sldelay.year, INT_TYPE, 1),
	MAKE_VARIABLE (".delay_hour", & sldelay.hour, INT_TYPE, 1),
	MAKE_VARIABLE (".delay_min", & sldelay.min, INT_TYPE, 1),
	MAKE_VARIABLE (".delay_sec", & sldelay.sec, INT_TYPE, 1),
	MAKE_VARIABLE (".expire_day", & slexpire.day, INT_TYPE, 1),
	MAKE_VARIABLE (".expire_mon", & slexpire.mon, INT_TYPE, 1),
	MAKE_VARIABLE (".expire_year", & slexpire.year, INT_TYPE, 1),
	MAKE_VARIABLE (".expire_hour", & slexpire.hour, INT_TYPE, 1),
	MAKE_VARIABLE (".expire_min", & slexpire.min, INT_TYPE, 1),
	MAKE_VARIABLE (".expire_sec", & slexpire.sec, INT_TYPE, 1),
	MAKE_VARIABLE (".rds", & slrds, INT_TYPE, 1),
	MAKE_VARIABLE (".False", & xFalse, INT_TYPE, 1),
	MAKE_VARIABLE (".True", & xTrue, INT_TYPE, 1),
	MAKE_VARIABLE (".Null_String", & xNull_String, STRING_TYPE, 1),
/*
	MAKE_INTRINSIC (".", , _TYPE, 0),
	MAKE_INTRINSIC (".function_name", c_function, TYPE, 0),
	MAKE_VARIABLE (".", & , TYPE, ),
	MAKE_VARIABLE (".var", &c_variable, TYPE, flag),
*/
        SLANG_END_TABLE
};
/*}}}*/
/*{{{	init/deinit */
static int
slang_init (script *s, char *libdir)
{
	char	*fname;
	
	if (! isinit) {
		if ((! init_SLang ()) || (! init_SLmath ()) ||
		    (! init_SLunix ()) || (! init_SLfiles ()) ||
		    (! string_class ()) || (! SLang_add_table (avail, "yaps")))
			return -1;
		if (libdir && (fname = malloc (strlen (libdir) + sizeof (STARTUP) + 4))) {
			sprintf (fname, "%s/%s", libdir, STARTUP);
			if (access (fname, R_OK) != -1)
				if ((! SLang_load_file (fname)) || SLang_Error)
					SLang_restart (1);
			free (fname);
		}
		isinit = True;
	}
	if (slline)
		slline[0] = '\0';
	return NO_ERR;
}

static void
slang_deinit (script *s)
{
	if (slline) {
		free (slline);
		slline = NULL;
	}
	slsiz = 0;
}
/*}}}*/
/*{{{	execute */
static int
slang_execute (script *s, char *label, char *parm)
{
	SLang_Name_Type	*func;
	int		ret;
	
	ret = NO_ERR;
	if (func = SLang_get_function (label)) {
		SLang_push_string (parm ? parm : "");
		sldelay = s -> delay;
		slexpire = s -> expire;
		slrds = s -> rds;
		sls = s;
		SLexecute_function (func);
		if (sls -> sp)
			tty_set_line_callback (sls -> sp, NULL, NULL, NULL);
		sls = NULL;
		if (SLang_Error || SLang_pop_integer (& ret)) {
			ret = ERR_FATAL;
			SLang_restart (1);
		}
	}
	return ret;
}
/*}}}*/
/*{{{	loading */
static int
slang_load_string (script *s, char *scr)
{
	int	err;
	
	err = ERR_FATAL;
	if (SLang_load_string (scr) && (! SLang_Error))
		err = NO_ERR;
	else
		SLang_restart (1);
	return err;
}

static int
slang_load_file (script *s, char *fname)
{
	int	err;

	err = ERR_FATAL;
	if (SLang_load_file (fname) && (! SLang_Error))
		err = NO_ERR;
	else
		SLang_restart (1);
	return err;
}
/*}}}*/
/*{{{	preinit/postdeinit/scriptentry */
static int
slang_preinit (char *libdir)
{
	return slang_init (NULL, libdir);
}

static void
slang_postdeinit (void)
{
	slang_deinit (NULL);
}

funcs	fslang = {
	"SLang",
	slang_init,
	slang_deinit,
	slang_execute,
	slang_load_string,
	slang_load_file,
	slang_preinit,
	slang_postdeinit
};
/*}}}*/
# endif		/* SCRIPT_SLANG */
