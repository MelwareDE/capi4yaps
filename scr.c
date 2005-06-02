/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# include	<stdio.h>
# include	<stdlib.h>
# include	<string.h>
# include	"pager.h"
# include	"script.h"

/*{{{	available scripts */
static funcs	*flist[] = {
# ifdef		SCRIPT_SLANG	
	& fslang,
# endif		/* SCRIPT_SLANG */
# ifdef		SCRIPT_LUA
	& flua,
# endif		/* SCRIPT_LUA */
	NULL
};
/*}}}*/
/*{{{	convert */
char *
scr_convert (script *s, char *str)
{
	char	*ret;
	int	n, m;
	int	c;
	
	if (ret = malloc (strlen (str) + 1)) {
		for (n = 0, m = 0; str[n]; ++n)
			if ((c = cv_conv (s -> ctab, (char_t) str[n])) > 0)
				ret[m++] = (char) c;
		ret[m] = '\0';
	}
	return ret;
}
/*}}}*/
/*{{{	execute */
int
scr_execute (void *sp, char *label, char *parm)
{
	script	*s = (script *) sp;
	
	MCHK (s);
	if ((! s) || (! s -> f) || (! s -> f -> fexec))
		return ERR_FATAL;
	return (*s -> f -> fexec) (s, label, parm);
}
/*}}}*/
/*{{{	script loading */
int
scr_load_string (void *sp, char *scr)
{
	script	*s = (script *) sp;

	MCHK (s);
	if ((! s) || (! s -> f) || (! s -> f -> fsload))
		return ERR_FATAL;
	return (*s -> f -> fsload) (s, scr);
}

int
scr_load_file (void *sp, char *fname)
{
	script	*s = (script *) sp;

	MCHK (s);
	if ((! s) || (! s -> f) || (! s -> f -> ffload))
		return ERR_FATAL;
	return (*s -> f -> ffload) (s, fname);
}
/*}}}*/
/*{{{	configuration */
void
scr_config (void *sp, void (*logger) (char, char *, ...), date_t *delay, date_t *expire, Bool rds)
{
	script	*s = (script *) sp;
	
	MCHK (s);
	if (s) {
		s -> logger = logger;
		if (delay)
			s -> delay = *delay;
		else
			dat_clear (& s -> delay);
		if (expire)
			s -> expire = *expire;
		else
			dat_clear (& s -> expire);
		s -> rds = False;
	}
}

void
scr_set_convtable (void *sp, void *ctab)
{
	script	*s = (script *) sp;
	
	MCHK (s);
	if (s) {
		if (s -> ctab)
			cv_free (s -> ctab);
		s -> ctab = ctab;
	}
}

void
scr_add_convtable (void *sp, void *ctab)
{
	script	*s = (script *) sp;

	MCHK (s);
	if (s) {
		if (! s -> ctab)
			s -> ctab = cv_new ();
		if (s -> ctab)
			cv_merge (s -> ctab, ctab, True);
	}
}
/*}}}*/
/*{{{	new/free/etc */
void *
scr_new (void *sp, char *typ, char *libdir)
{
	script	*s;
	int	n;

	if (s = (script *) malloc (sizeof (script))) {
# ifndef	NDEBUG
		s -> magic = MAGIC;
# endif		/* NDEBUG */
		s -> sp = sp;
		s -> ctab = NULL;
		s -> logger = NULL;
		dat_clear (& s -> delay);
		dat_clear (& s -> expire);
		s -> rds = False;
		s -> priv = NULL;
		for (n = 0; flist[n]; ++n)
			if (flist[n] -> typ && (! strcmp (flist[n] -> typ, typ)))
				break;
		if (flist[n]) {
			s -> f = flist[n];
			if (s -> f -> finit)
				if ((*s -> f -> finit) (s, libdir) < 0)
					s = scr_free (s);
		} else
			s = scr_free (s);
	}
	return s;
}

void *
scr_free (void *sp)
{
	script	*s = (script *) sp;
	
	MCHK (s);
	if (s) {
		if (s -> f && s -> f -> fdeinit)
			(*s -> f -> fdeinit) (s);
		if (s -> ctab)
			cv_free (s -> ctab);
		free (s);
	}
	return NULL;
}

int
scr_preinit (char *libdir)
{
	int	n;
	
	for (n = 0; flist[n]; ++n)
		if (flist[n] -> fpreinit)
			if ((*flist[n] -> fpreinit) (libdir) < 0)
				return -1;
	return 0;
}

void
scr_postdeinit (void)
{
	int	n;
	
	for (n = 0; flist[n]; ++n)
		if (flist[n] -> fpostdeinit)
			(*flist[n] -> fpostdeinit) ();
				
}
/*}}}*/
