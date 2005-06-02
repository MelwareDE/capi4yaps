/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# ifdef		SCRIPT_LUA
# include	<stdio.h>
# include	<stdlib.h>
# include	<unistd.h>
# include	<string.h>
# include	<lua.h>
# include	<lualib.h>
# include	"pager.h"
# include	"script.h"

# define	STARTUP		"Startup.lua"

/*{{{	statics & callable functions */
static Bool	isinit = False;
static double	lua_no_err = (double) NO_ERR,
		lua_err_fail = (double) ERR_FAIL,
		lua_err_fatal = (double) ERR_FATAL,
		lua_err_abort = (double) ERR_ABORT;
static script	*ls = NULL;
static char	*lline = NULL;
static int	lsiz = 0;
static char	*lcb = NULL;

static void
lua_logger (void)
{
	lua_Object	obj;
	char		*sav, *str;
	char		typ;
	
	if (((obj = lua_getparam (1)) != LUA_NOOBJECT) &&
	    (str = lua_getstring (obj)) &&
	    (sav = strdup (str))) {
		typ = *sav;
		if (((obj = lua_getparam (2)) == LUA_NOOBJECT) ||
		    (! (str = lua_getstring (obj)))) {
			str = sav;
			typ = LG_INF;
		}
		if (ls && ls -> logger)
			(*ls -> logger) (typ, "%s\n", str);
		free (sav);
	}
}

static void
lua_callback (void *sp, string_t *s, char_t sep, void *data)
{
	int		len;
	char		*str;
	lua_Object	obj;

	if (str = sextract (s)) {
		len = strlen (str);
		if (len + 2 >= lsiz) {
			lsiz = len + 64;
			if (! (lline = Realloc (lline, lsiz + 4)))
				lsiz = 0;
		}
		if (lline) {
			sprintf (lline, "%s%c", str, (char) sep);
			if (lcb && (obj = lua_getglobal (lcb)) && lua_isfunction (obj)) {
				lua_pushstring (lline);
				lua_callfunction (obj);
			}
		}
		free (str);
	}
}

static void
lua_setcb (void)
{
	lua_Object	obj1, obj2;
	char		*sep, *func;
	
	if ((obj1 = lua_getparam (1)) != LUA_NOOBJECT)
		obj2 = lua_getparam (2);
	else
		obj2 = LUA_NOOBJECT;
	if (lcb) {
		free (lcb);
		lcb = NULL;
	}
	if ((obj1 == LUA_NOOBJECT) || (! lua_isstring (obj1)) || (! (sep = lua_getstring (obj1)))) {
		if (ls && ls -> sp)
			tty_set_line_callback (ls -> sp, NULL, NULL, NULL);
	} else {
		if (ls && ls -> sp)
			tty_set_line_callback (ls -> sp, lua_callback, sep, NULL);
		if ((obj2 != LUA_NOOBJECT) && lua_isstring (obj2) && (func = lua_getstring (obj2)))
			lcb = strdup (func);
	}
}

static void
lua_get_line (void)
{
	if (lline)
		lua_pushstring (lline);
	else
		lua_pushnil ();
}

static void
lua_hangup (void)
{
	lua_Object	obj;
	double		sec;
	int		msec;
	
	if (((obj = lua_getparam (1)) != LUA_NOOBJECT) &&
	    lua_isnumber (obj)) {
		sec = lua_getnumber (obj);
		msec = (int) (sec * 1000.0);
	} else
		msec = 500;
	if (ls && ls -> sp)
		tty_hangup (ls -> sp, msec);
}

static void
do_send (Bool dcv)
{
	int		ret;
	lua_Object	obj;
	char		*str;
	
	ret = 0;
	if (((obj = lua_getparam (1)) != LUA_NOOBJECT) &&
	    lua_isstring (obj) && (str = lua_getstring (obj)))
		if (ls && ls -> sp) {
			if (dcv)
				str = scr_convert (ls, str);
			if (str) {
				if (tty_send_string (ls -> sp, str) != -1)
					ret = 1;
				if (dcv)
					free (str);
			}
		}
	if (ret)
		lua_pushnumber (1.0);
	else
		lua_pushnil ();
}

static void
lua_send (void)
{
	do_send (False);
}

static void
lua_csend (void)
{
	do_send (True);
}

static void
lua_expect (void)
{
	int		ret;
	lua_Object	obj;
	int		tout;
	char		*str;
	int		cnt, siz;
	char		**ex;
	int		*len;
	int		start;
	int		n;
	
	if (((obj = lua_getparam (1)) != LUA_NOOBJECT) && lua_isnumber (obj)) {
		tout = (int) lua_getnumber (obj);
		start = 2;
	} else {
		tout = 5;
		start = 1;
	}
	ex = NULL;
	cnt = 0;
	siz = 0;
	while (((obj = lua_getparam (start)) != LUA_NOOBJECT) &&
	       lua_isstring (obj) && (str = lua_getstring (obj))) {
		if (cnt >= siz) {
			siz += 4;
			if (! (ex = (char **) Realloc (ex, (siz + 2) * sizeof (char *))))
				break;
		}
		if (ex[cnt] = strdup (str))
			++cnt;
	}
	ret = -1;
	if (ex) {
		ex[cnt] = NULL;
		if ((cnt > 0) && (len = (int *) malloc ((cnt + 1) * sizeof (int)))) {
			for (n = 0; n < cnt; ++n)
				len[n] = strlen (ex[n]);
			len[cnt] = 0;
			if (ls && ls -> sp)
				ret = tty_expect_list (ls -> sp, tout, ex, len);
			free (len);
			for (n = 0; n < cnt; ++n)
				free (ex[n]);
		}
		free (ex);
	}
	lua_pushnumber ((double) ret);
}

static void
lua_send_expect (void)
{
	int		ret;
	lua_Object	obj;
	int		tout;
	char		*str;

	ret = 0;
	if ((obj = lua_getparam (1)) != LUA_NOOBJECT) {
		if (lua_isnumber (obj)) {
			tout = (int) lua_getnumber (obj);
			obj = lua_getparam (2);
		}
		if ((obj != LUA_NOOBJECT) && lua_isstring (obj) && (str = lua_getstring (obj)))
			if (ls && ls -> sp && (tty_send_expect (ls -> sp, tout, str, NULL) != -1))
				ret = 1;
	}
	if (ret)
		lua_pushnumber (1.0);
	else
		lua_pushnil ();
}

static void
lua_drain (void)
{
	lua_Object	obj;
	int		sec;
	
	if (((obj = lua_getparam (1)) != LUA_NOOBJECT) && lua_isnumber (obj))
		sec = (int) lua_getnumber (obj);
	else
		sec = 1;
	if (ls && ls -> sp)
		tty_drain (ls -> sp, sec);
}

static void
lua_cvdef (void)
{
	lua_Object	obj1, obj2;
	char		*src, *dst;
	int		n;
	
	if (ls) {
		if (! ls -> ctab)
			ls -> ctab = cv_new ();
		if (ls -> ctab)
			if (((obj1 = lua_getparam (1)) != LUA_NOOBJECT) && lua_isstring (obj1) && (src = lua_getstring (obj1))) {
				if ((obj2 = lua_getparam (2)) == LUA_NOOBJECT)
					cv_undefine (ls -> ctab, (char_t) *src);
				else if (lua_isstring (obj2) && (dst = lua_getstring (obj2)))
					cv_define (ls -> ctab, (char_t) *src, (char_t) *dst);
				else if (lua_isnumber (obj2)) {
					n = (int) lua_getnumber (obj2);
					if (n < 0)
						cv_invalid (ls -> ctab, (char_t) *src);
					else
						cv_define (ls -> ctab, (char_t) *src, (char_t) n);
				}
			}
	}
}

static void
lua_conv (void)
{
	char		*ret;
	lua_Object	obj;
	char		*str;
	
	ret = NULL;
	if (((obj = lua_getparam (1)) != LUA_NOOBJECT) && lua_isstring (obj) && (str = lua_getstring (obj)))
		if (ls)
			ret = scr_convert (ls, str);
	if (ret) {
		lua_pushstring (ret);
		free (ret);
	} else
		lua_pushnil ();
}
/*}}}*/
/*{{{	init/deinit */
static int
lua_init (script *s, char *libdir)
{
	char	*fname;

	if (! isinit) {
		iolib_open ();
		strlib_open ();
		mathlib_open ();
		lua_pushnumber (lua_no_err);	lua_storeglobal ("NO_ERR");
		lua_pushnumber (lua_err_fail);	lua_storeglobal ("ERR_FAIL");
		lua_pushnumber (lua_err_fatal);	lua_storeglobal ("ERR_FATAL");
		lua_pushnumber (lua_err_abort);	lua_storeglobal ("ERR_ABORT");
		/* void		logger (string str);				*/
		lua_register ("logger", lua_logger);
		/* void		setcb ([string sep[, string|function func]]);	*/
		lua_register ("setcb", lua_setcb);
		/* string|nil	get_line (void);				*/
		lua_register ("get_line", lua_get_line);
		/* void		hangup ([num sec]);				*/
		lua_register ("hangup", lua_hangup);
		/* num|nil	send (string line);				*/
		lua_register ("send", lua_send);
		/* num|nil	csend (string line);				*/
		lua_register ("csend", lua_csend);
		/* num		expect (num tout, string, s1, ..., string sn);	*/
		lua_register ("expect", lua_expect);
		/* num|nil	send_expect (num tout, string str);		*/
		lua_register ("send_expect", lua_send_expect);
		/* void		drain ([num sec]);				*/
		lua_register ("drain", lua_drain);
		/* void		cvdef (string src[, string|num dst]);		*/
		lua_register ("cvdef", lua_cvdef);
		/* string|nil	conv (string str);				*/
		lua_register ("conv", lua_conv);
		if (libdir && (fname = malloc (strlen (libdir) + sizeof (STARTUP) + 4))) {
			sprintf (fname, "%s/%s", libdir, STARTUP);
			if (access (fname, R_OK) != -1)
				lua_dofile (fname);
			free (fname);
		}
		lline = NULL;
		lsiz = 0;
		isinit = True;
	}
	return NO_ERR;
}

static void
lua_deinit (script *s)
{
	if (lline) {
		free (lline);
		lline = NULL;
		lsiz = 0;
	}
}
/*}}}*/
/*{{{	execution */
static int
lua_execute (script *s, char *func, char *parm)
{
	int		err;
	lua_Object	obj;
	double		ret;
	
	err = NO_ERR;
	if ((obj = lua_getglobal (func)) && lua_isfunction (obj)) {
		ls = s;
		lua_beginblock ();
		lua_pushnumber ((double) s -> delay.day);	lua_storeglobal ("delay_day");
		lua_pushnumber ((double) s -> delay.mon);	lua_storeglobal ("delay_mon");
		lua_pushnumber ((double) s -> delay.year);	lua_storeglobal ("delay_year");
		lua_pushnumber ((double) s -> delay.hour);	lua_storeglobal ("delay_hour");
		lua_pushnumber ((double) s -> delay.min);	lua_storeglobal ("delay_min");
		lua_pushnumber ((double) s -> delay.sec);	lua_storeglobal ("delay_sec");
		lua_pushnumber ((double) s -> expire.day);	lua_storeglobal ("expire_day");
		lua_pushnumber ((double) s -> expire.mon);	lua_storeglobal ("expire_mon");
		lua_pushnumber ((double) s -> expire.year);	lua_storeglobal ("expire_year");
		lua_pushnumber ((double) s -> expire.hour);	lua_storeglobal ("expire_hour");
		lua_pushnumber ((double) s -> expire.min);	lua_storeglobal ("expire_min");
		lua_pushnumber ((double) s -> expire.sec);	lua_storeglobal ("expire_sec");
		lua_pushnumber ((double) s -> rds);		lua_storeglobal ("rds");
		if (parm)
			lua_pushstring (parm);
		else
			lua_pushnil ();
		if (lua_callfunction (obj))
			err = ERR_FATAL;
		else if ((obj = lua_getresult (1)) && (obj != LUA_NOOBJECT)) {
			ret = lua_getnumber (obj);
			if (ret == lua_no_err)
				err = NO_ERR;
			else if (ret == lua_err_fail)
				err = ERR_FAIL;
			else if (ret == lua_err_fatal)
				err = ERR_FATAL;
			else if (ret == lua_err_abort)
				err = ERR_ABORT;
		}
		lua_endblock ();
		if (ls -> sp)
			tty_set_line_callback (ls -> sp, NULL, NULL, NULL);
		ls = NULL;
	}
	return err;
}
/*}}}*/
/*{{{	loading */
static int
lua_load_string (script *s, char *scr)
{
	return lua_dostring (scr) ? ERR_FATAL : NO_ERR;
}

static int
lua_load_file (script *s, char *fname)
{
	return lua_dofile (fname) ? ERR_FATAL : NO_ERR;
}
/*}}}*/
/*{{{	preinit/postdeinit/scriptentry */
static int
lua_preinit (char *libdir)
{
	return lua_init (NULL, libdir);
}

static void
lua_postdeinit (void)
{
	lua_deinit (NULL);
}

funcs	flua = {
	"Lua",
	lua_init,
	lua_deinit,
	lua_execute,
	lua_load_string,
	lua_load_file,
	lua_preinit,
	lua_postdeinit
};
/*}}}*/
# endif		/* SCRIPT_LUA */
