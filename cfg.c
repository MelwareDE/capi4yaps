/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# include	<stdio.h>
# include	<stdlib.h>
# include	<ctype.h>
# include	<string.h>
# include	"pager.h"

/*{{{	typedefs */
typedef unsigned long	hash_t;

typedef struct _entry {
	hash_t	hval;
	char	*var;
	char	*val;
	struct _entry
		*next;
}	entry;

typedef struct _charc {
	hash_t	hval;
	char	*name;
	struct _charc
		*next;
}	charc;

typedef struct _block {
# ifndef	NDEBUG
# define	MAGIC		MKMAGIC ('c', 'f', 'g', '\0')
	long	magic;
# endif		/* NDEBUG */
	hash_t	hval;
	char	*name;
	charc	*use;
	Bool	done;
	entry	*e;
	char	*sep;
	int	seplen;
	struct _block
		*next;
}	block;

typedef struct _fifo {
	void	*dat;
	struct _fifo
		*next;
}	fifo;

typedef struct _fpstack {
	FILE	*fp;
	struct _fpstack
		*up;
}	fpstack;
/*}}}*/
/*{{{	support routines */
static hash_t
calc_hash (char *str)
{
	hash_t	hval;
	
	for (hval = 0; *str; ) {
		hval <<= 2;
		hval += *str++ & 0xff;
		hval >>= 1;
	}
	return hval;
}

static Bool
issame (hash_t h1, char *s1, hash_t h2, char *s2)
{
	if (h1 == h2)
		if ((! s1) && (! s2))
			return True;
		else if (s1 && s2 && (s1[0] == s2[0]) && (! strcmp (s1, s2)))
			return True;
	return False;
}

static block *
new_block (char *bname, hash_t hval, char *sep)
{
	block	*b;
	
	if (b = (block *) malloc (sizeof (block))) {
		b -> name = NULL;
		if ((! bname) || (b -> name = strdup (bname))) {
			b -> sep = NULL;
			b -> seplen = 0;
			if ((! sep) || (b -> sep = strdup (sep))) {
# ifndef	NDEBUG
				b -> magic = MAGIC;
# endif		/* NDEBUG */
				b -> hval = hval;
				b -> use = NULL;
				b -> done = False;
				b -> e = NULL;
				b -> next = NULL;
			} else {
				if (b -> name)
					free (b -> name);
				free (b);
				b = NULL;
			}
		} else {
			free (b);
			b = NULL;
		}
	}
	return b;
}

static void
add_entry (block *base, block *b, char *var, char *val)
{
	hash_t	hval;
	entry	*e, *etmp;
	Bool	add;
	char	*sav;

	if (b && var) {
		if (*var == '+') {
			++var;
			add = True;
		} else
			add = False;
		hval = calc_hash (var);
		for (e = b -> e; e; e = e -> next)
			if (issame (hval, var, e -> hval, e -> var))
				break;
		if (e) {
			if (e -> val)
				if (add) {
					if (val && *val) {
						sav = e -> val;
						if (e -> val = malloc (strlen (sav) + strlen (val) + base -> seplen + 2)) {
							sprintf (e -> val, "%s%s%s", sav, (base -> sep ? base -> sep : ""), val);
							free (sav);
							val = NULL;
						} else
							e -> val = sav;
					}
				} else {
					free (e -> val);
					e -> val = NULL;
				}
		} else if (e = (entry *) malloc (sizeof (entry)))
			if (e -> var = strdup (var)) {
				e -> hval = hval;
				e -> val = NULL;
				e -> next = b -> e;
				b -> e = e;
			} else {
				free (e);
				e = NULL;
			}
		if (e && (! e -> val) && val && *val) {
			if (add && (base != b)) {
				for (etmp = base -> e; etmp; etmp = etmp -> next)
					if (issame (hval, var, etmp -> hval, etmp -> var))
						break;
				if (etmp && etmp -> val)
					if (e -> val = malloc (strlen (etmp -> val) + strlen (val) + base -> seplen + 2))
						sprintf (e -> val, "%s%s%s", etmp -> val, (base -> sep ? base -> sep : ""), val);
			}
			if (! e -> val)
				e -> val = strdup (val);
		}
	}
}
/*}}}*/
/*{{{	read configuration */
void *
cfg_new (char *sep)
{
	return (void *) new_block (NULL, 0, sep);
}

void *
cfg_read (char *fname, void *bp, char *sep)
{
	FILE	*fp;
	fpstack	*fcur, *ftmp;
	char	*gline, *line;
	char	*ptr, *use;
	charc	*prv, *tmp;
	block	*base, *prev, *cur;
	hash_t	hval;
	char	*var, *val;
	int	plen;
	Bool	done;
	int	siz, len;
	char	*temp;
	
	if (bp)
		base = (block *) bp;
	else
		if (! (base = new_block (NULL, 0, sep)))
			return NULL;
	cur = base;
	if (fp = fopen (fname, "r"))
		if (fcur = (fpstack *) malloc (sizeof (fpstack))) {
			fcur -> fp = fp;
			fcur -> up = NULL;
			while (fcur) {
				while (gline = getline (fcur -> fp, True)) {
					for (line = gline; isspace (*line); ++line)
						;
					if ((! *line) || (*line == '#')) {
						free (gline);
						continue;
					}
					if (*line == '[') {
						use = NULL;
						if (ptr = strchr (line, ']')) {
							*ptr++ = '\0';
							while (isspace (*ptr))
								++ptr;
							if (*ptr)
								use = ptr;
						}
						ptr = line + 1;
						if (! *ptr)
							cur = base;
						else {
							hval = calc_hash (ptr);
							for (cur = base, prev = NULL; cur; cur = cur -> next)
								if (issame (hval, ptr, cur -> hval, cur -> name))
									break;
								else
									prev = cur;
							if ((! cur) && (cur = new_block (ptr, hval, NULL)))
								if (prev)
									prev -> next = cur;
							if (cur && use) {
								if (cur -> use)
									for (prv = cur -> use; prv -> next; prv = prv -> next)
										;
								else
									prv = NULL;
								while (*use) {
									ptr = use;
									use = skipch (ptr, ',');
									if (tmp = (charc *) malloc (sizeof (charc)))
										if (tmp -> name = strdup (ptr)) {
											tmp -> hval = 0;
											tmp -> next = NULL;
											if (prv)
												prv -> next = tmp;
											else
												cur -> use = tmp;
											prv = tmp;
										} else
											free (tmp);
								}
							}
						}
						if (! cur) {
							free (gline);
							break;
						}
					} else if (*line == '|') {
						++line;
						while (isspace (*line))
							++line;
						if (fp = fopen (line, "r"))
							if (ftmp = (fpstack *) malloc (sizeof (fpstack))) {
								ftmp -> fp = fp;
								ftmp -> up = fcur;
								fcur = ftmp;
							} else
								fclose (fp);
					} else {
						var = line;
						val = skip (var);
						if (var = strdup (var)) {
							temp = NULL;
							if ((*val == '{') && (! *(val + 1))) {
								done = False;
								siz = 0;
								len = 0;
								while (ptr = getline (fcur -> fp, False)) {
									if ((*ptr != '}') || *(ptr + 1)) {
										plen = strlen (ptr);
										if (len + plen + 2 >= siz) {
											siz = len + plen + 256;
											if (! (temp = Realloc (temp, siz + 2))) {
												siz = 0;
												done = True;
											}
										}
										if (len + plen < siz) {
											if (len)
												temp[len++] = '\n';
											strcpy (temp + len, ptr);
											len += plen;
										}
									} else
										done = True;
									free (ptr);
									if (done)
										break;
								}
								if (temp) {
									temp[len] = '\0';
									val = temp;
								} else
									val = NULL;
							} else if (*val == '\\')
								++val;
							add_entry (base, cur, var, val);
							if (temp)
								free (temp);
							free (var);
						}
					}
					free (gline);
				}
				ftmp = fcur;
				fcur = fcur -> up;
				fclose (ftmp -> fp);
				free (ftmp);
			}
		} else
			fclose (fp);
	return (void *) base;
}
/*}}}*/
/*{{{	add/change configuration */
void
cfg_modify (void *bp, char *bname, char *var, char *val)
{
	block	*base = (block *) bp;
	block	*use, *prv;
	hash_t	hval;

	MCHK (base);
	if (base) {
		if (! bname)
			use = base;
		else {
			hval = calc_hash (bname);
			for (use = base -> next, prv = base; use; use = use -> next)
				if (issame (hval, bname, use -> hval, use -> name))
					break;
				else
					prv = use;
			if (! use)
				use = new_block (bname, hval, NULL);
		}
		if (use)
			add_entry (base, use, var, val);
	}
}
/*}}}*/
/*{{{	end (free up) configuration */
void *
cfg_end (void *bp)
{
	block	*b = (block *) bp;
	block	*tmp;
	charc	*run;
	entry	*e;
	
	MCHK (b);
	while (b) {
		if (b -> name)
			free (b -> name);
		while (b -> use) {
			run = b -> use;
			b -> use = b -> use -> next;
			if (run -> name)
				free (run -> name);
			free (run);
		}
		while (b -> e) {
			e = b -> e;
			b -> e = b -> e -> next;
			if (e -> var)
				free (e -> var);
			if (e -> val)
				free (e -> val);
			free (e);
		}
		if (b -> sep)
			free (b -> sep);
		tmp = b;
		b = b -> next;
		free (tmp);
	}
	return NULL;
}
/*}}}*/
/*{{{	retrieving */
static char *
do_get (void *bp, char *bname, char *var, Bool glob, char *dflt)
{
	block	*b = (block *) bp;
	block	*run, *tmp;
	entry	*e;
	fifo	*f, *l, *ft;
	charc	*use;
	char	*ptr, *sav;
	charc	*vars, *vprv, *vtmp, *vrun;
	Bool	first;
	hash_t	hval;
	
	MCHK (b);
	if (! (var = strdup (var)))
		return NULL;
	vars = NULL;
	vprv = NULL;
	for (ptr = var; *ptr; ) {
		sav = ptr;
		ptr = skipch (ptr, ',');
		if (vtmp = (charc *) malloc (sizeof (charc)))
			if (vtmp -> name = strdup (sav)) {
				vtmp -> hval = calc_hash (vtmp -> name);
				vtmp -> next = NULL;
				if (vprv)
					vprv -> next = vtmp;
				else
					vars = vtmp;
				vprv = vtmp;
			} else
				free (vtmp);
	}
	free (var);
	if (! vars)
		return NULL;
	e = NULL;
	f = NULL;
	l = NULL;
	first = True;
	while ((! e) && bname) {
		hval = calc_hash (bname);
		for (run = b -> next; run; run = run -> next)
			if (issame (hval, bname, run -> hval, run -> name))
				break;
		bname = NULL;
		if (run && (first || (! run -> done))) {
			for (vrun = vars; vrun; vrun = vrun -> next) {
				for (e = run -> e; e; e = e -> next)
					if (issame (vrun -> hval, vrun -> name, e -> hval, e -> var))
						break;
				if (e)
					break;
			}
			if (! e) {
				if (use = run -> use) {
					if (first) {
						for (tmp = b -> next; tmp; tmp = tmp -> next)
							tmp -> done = False;
						first = False;
					}
					for (; use; use = use -> next)
						if (ft = (fifo *) malloc (sizeof (fifo))) {
							ft -> dat = (void *) use;
							ft -> next = NULL;
							if (l)
								l -> next = ft;
							else {
								f = ft;
								l = ft;
							}
						} else
							break;
				}
				if (f) {
					ft = f;
					f = f -> next;
					if (! f)
						l = NULL;
					use = (charc *) ft -> dat;
					bname = use -> name;
					free (ft);
				} else
					bname = NULL;
				run -> done = True;
			}
		}
	}
	if ((! e) && glob)
		for (vrun = vars; vrun; vrun = vrun -> next) {
			for (e = b -> e; e; e = e -> next)
				if (issame (vrun -> hval, vrun -> name, e -> hval, e -> var))
					break;
			if (e)
				break;
		}
	while (vars) {
		vtmp = vars;
		vars = vars -> next;
		free (vtmp -> name);
		free (vtmp);
	}
	return e ? e -> val : dflt;
}

static int
do_iget (void *bp, char *bname, char *var, Bool glob, int dflt)
{
	char	*ret;
	
	ret = do_get (bp, bname, var, glob, NULL);
	return ret ? atoi (ret) : dflt;
}

static Bool
do_bget (void *bp, char *bname, char *var, Bool glob, int dflt)
{
	char	*ret;
	
	ret = do_get (bp, bname, var, glob, NULL);
	return ret ? ((*ret && strchr ("TtYy1+", *ret)) ? True : False) : dflt;
}

char *
cfg_get (void *bp, char *bname, char *var, char *dflt)
{
	return do_get (bp, bname, var, True, dflt);
}

int
cfg_iget (void *bp, char *bname, char *var, int dflt)
{
	return do_iget (bp, bname, var, True, dflt);
}

Bool
cfg_bget (void *bp, char *bname, char *var, Bool dflt)
{
	return do_bget (bp, bname, var, True, dflt);
}

char *
cfg_block_get (void *bp, char *bname, char *var, char *dflt)
{
	return do_get (bp, bname, var, False, dflt);
}

int
cfg_block_iget (void *bp, char *bname, char *var, int dflt)
{
	return do_iget (bp, bname, var, False, dflt);
}

Bool
cfg_block_bget (void *bp, char *bname, char *var, Bool dflt)
{
	return do_bget (bp, bname, var, False, dflt);
}
/*}}}*/
