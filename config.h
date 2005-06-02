/*	-*- mode: c; mode: fold -*-	*/
# ifndef	__CONFIG_H
# define	__CONFIG_H		1

/*{{{	changeable configuration 					*/
/*
 * Define signal handling:
 * POSIX_SIGNAL		if you have the Posix sigaction() family
 * BSD_SIGNAL		if you have BSD like signal() handling
 * SYSV_SIGNAL		if you have SysV like signal() handling
 * SIG_VOID_RETURN	if your signal handler returns void
 * SIG_INT_RETURN	if your signal handler returns int
 */
# define	POSIX_SIGNAL		1
# define	BSD_SIGNAL		0
# define	SYSV_SIGNAL		0
# define	SIG_VOID_RETURN		1
# define	SIG_INT_RETURN		0

/*
 * Set each define to 1, if you have the matching header file, otherwise
 * set it to 0. Remember, that some features may not available, if the
 * header file is not available.
 */
/*
 * Needed only by some systems, which do not define FD_SET etc.
 * in sys/time.h
 */
# define	HAVE_SYS_SELECT_H	0

/*
 * If you have locales set this. This is useful to for character
 * conversion/classification
 */
# define	HAVE_LOCALE_H		0

/*
 * If you have Posix regular expressions, set this. Otherwise a
 * very weak replacement is used to find matching services
 */
# define	HAVE_REGEX_H		0

/*
 * one of these is required for SysV like lockfiles
 */
# define	HAVE_SYS_SYSMACROS_H	1
# define	HAVE_SYS_MKDEV_H	0

/*
 * Some system do not define the getopt stuff in unistd.h, but in
 * a own include file getopt.h. Or (like the GNU libc) defines there
 * the extended getopt_long version.
 */
# define	HAVE_GETOPT_H		0

/*
 * Set each define to 1, if your library supports the function, otherwise
 * set it to 0. See above for note.
 */
/*
 * If the library contains this function, a call to it is required
 * to get valid return values from localtime
 */
# define	HAVE_TZSET		0

/*
 * If these are not set, chmod()/chown() are used
 */
# define	HAVE_FCHMOD		0
# define	HAVE_FCHOWN		0

/*
 * If you have sigsetjmp() you definitly want to set this, otherwise 
 * longjmp() from the signal handler leads into chaos
 */
# define	HAVE_SIGSETJMP		0

/*
 * Memory access functions. Nearly everybody has memcpy()/memset(), so
 * choose the bcopy()/bzero() part only if you are missing the other two
 */
# define	HAVE_MEMCPY		1
# define	HAVE_BCOPY		0
# define	HAVE_MEMSET		1
# define	HAVE_BZERO		0

/*
 * If your library supports getopt at all
 */
# define	HAVE_GETOPT		1

/*
 * If your library supports long options (getopt_long(3)), then set this
 * to one
 */
# define	HAVE_GETOPT_LONG	0

/*
 * If you have getopt(3), but your headerfile(s) does not declare
 * optind/optarg set this to 1, otherwise to 0
 */
# define	NEED_OPTIND_OPTARG	0

/*
 * If your realloc(3) function cannot handle realloc (NULL, size), then
 * set this to 1, otherwise to 0
 */
# define	BROKEN_REALLOC		1

/*      -------------- END OF CHANGEABLE PART ------------------	*/
/*}}}*/
/*{{{	auto configuration part						*/
/*
 * Autoconfiguration
 */
# if		! HAVE_MEMCPY
#  if		HAVE_BCOPY
#   define	memcpy(aa,bb,cc)	bcopy((bb),(aa),(cc))
#  else		/* HAVE_BCOPY */
#   error	"Neither memcopy() nor bcopy() available, aborted"
#  endif	/* HAVE_BCOPY */
# endif		/* BSD */

# if		BROKEN_REALLOC
#  define	Realloc(ppp,sss)	((ppp) ? realloc ((ppp), (sss)) : malloc ((sss)))
# else		/* BROKEN_REALLOC */
#  define	Realloc			realloc
# endif		/* BROKEN_REALLOC */

# ifndef	__GCC__
# define	inline
# endif		/* __GCC__ */
/*}}}*/
# endif		/* __CONFIG_H */
