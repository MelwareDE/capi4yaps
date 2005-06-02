/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# include	<stdio.h>
# include	<stdlib.h>
# include	<ctype.h>
# include	"pager.h"

/*{{{	typedefs */
typedef struct {
# ifndef	NDEBUG
# define	MAGIC		MKMAGIC ('c', 'o', 'n', 'v')
	long	magic;
# endif		/* NDEBUG */
	int	c[256];		/* the conversion table			*/
}	conv;
/*}}}*/
/*{{{	new/free */
void *
cv_new (void)
{
	conv	*c;
	int	n;
	
	if (c = (conv *) malloc (sizeof (conv))) {
# ifndef	NDEBUG
		c -> magic = MAGIC;
# endif		/* NDEBUG */
		for (n = 0; n < 256; ++n)
			c -> c[n] = n;
	}
	return (void *) c;
}

void *
cv_free (void *cv)
{
	conv	*c = (conv *) cv;

	MCHK (c);
	if (c)
		free (c);
	return NULL;
}
/*}}}*/
/*{{{	reverse */
void *
cv_reverse (void *src)
{
	conv	*s = (conv *) src;
	conv	*c;
	int	n;
	
	MCHK (s);
	if (c = (conv *) cv_new ())
		for (n = 0; n < 256; ++n)
			if (s -> c[n] != -1)
				c -> c[s -> c[n]] = n;
	return (void *) c;
}
/*}}}*/
/*{{{	define/undefine */
static char_t
getval (char *str)
{
	char_t	ret;
	
	if (isdigit (*str))
		ret = (char_t) strtol (str, NULL, 0);
	else if (! *(str + 1))
		ret = (char_t) *str;
	else if (*str == '^') {
		++str;
		ret = (char_t) (*str == '?' ? 0x7f : (*str & 0x1f));
	} else if (*str == '\\') {
		++str;
		switch (*str) {
		case 'a':	ret = (char_t) '\a';	break;
		case 'b':	ret = (char_t) '\b';	break;
		case 'e':	ret = (char_t) '\x1b';	break;
		case 'f':	ret = (char_t) '\f';	break;
		case 'l':	ret = (char_t) '\012';	break;
		case 'n':	ret = (char_t) '\n';	break;
		case 'r':	ret = (char_t) '\r';	break;
		case 's':	ret = (char_t) ' ';	break;
		case 't':	ret = (char_t) '\t';	break;
		case 'v':	ret = (char_t) '\v';	break;
		default:	ret = (char_t) *str;	break;
		}
	} else
		ret = (char_t) *str;
	return ret;
}

void
cv_define (void *cv, char_t src, char_t dst)
{
	conv	*c = (conv *) cv;
	
	MCHK (c);
	if (c)
		c -> c[src] = dst;
}

void
cv_sdefine (void *cv, char *src, char *dst)
{
	cv_define (cv, getval (src), getval (dst));
}

void
cv_undefine (void *cv, char_t ch)
{
	conv	*c = (conv *) cv;
	
	MCHK (c);
	if (c)
		c -> c[ch] = ch;
}

void
cv_sundefine (void *cv, char *ch)
{
	cv_undefine (cv, getval (ch));
}

void
cv_invalid (void *cv, char_t ch)
{
	conv	*c = (conv *) cv;
	
	MCHK (c);
	if (c)
		c -> c[ch] = -1;
}

void
cv_sinvalid (void *cv, char *ch)
{
	cv_invalid (cv, getval (ch));
}
/*}}}*/
/*{{{	read/write table */
int
cv_read_table (void *cv, char *fname)
{
	conv	*c = (conv *) cv;
	FILE	*fp;
	char	*line;
	char	*sp, *dp;
	
	MCHK (c);
	if ((! c) || (! (fp = fopen (fname, "r"))))
		return -1;
	while (line = getline (fp, True)) {
		for (sp = line; isspace (*sp); ++sp)
			;
		if (*sp && (*sp != '#')) {
			dp = skip (sp);
			skip (dp);
			if (*sp)
				if (*dp)
					c -> c[getval (sp)] = getval (dp);
				else
					c -> c[getval (sp)] = -1;
			
		}
		free (line);
	}
	fclose (fp);
	return 0;
}

int
cv_write_table (void *cv, char *fname)
{
	conv	*c = (conv *) cv;
	FILE	*fp;
	int	n;
	
	MCHK (c);
	if ((! c) || (! (fp = fopen (fname, "w"))))
		return -1;
	fprintf (fp, "#\tThis file is generated automatically\n");
	for (n = 0; n < 256; ++n) {
		if ((! (n & 0x80)) && isprint (n))
			fprintf (fp, "#\t%c\n", (char) n);
		if (c -> c[n] != -1)
			fprintf (fp, "0x%02x\t0x%02x\n", n, c -> c[n]);
		else
			fprintf (fp, "0x%02x\n", n);
	}
	fclose (fp);
	return 0;
}
/*}}}*/
/*{{{	merging */
void
cv_merge (void *cv, void *in, Bool second)
{
	conv	*c = (conv *) cv,
		*i = (conv *) in;
	int	n;

	MCHK (c);
	MCHK (i);
	if (c && i)
		for (n = 0; n < 256; ++n)
			if (second || (c -> c[n] == n))
				c -> c[n] = i -> c[n];
}
/*}}}*/
/*{{{	converting */
int
cv_conv (void *cv, char_t ch)
{
	conv	*c = (conv *) cv;
	
	MCHK (c);
	return c ? c -> c[ch] : (int) ch;
}
/*}}}*/
