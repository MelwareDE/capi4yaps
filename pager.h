/*	-*- mode: c; mode: fold -*-	*/
# ifndef	__PAGER_H
# define	__PAGER_H		1
# include	<stdio.h>

/*{{{	definitions, macros */
# ifndef	NDEBUG
# define	V(lvl,msg)	((void) (((lvl) <= verbose) && verbout ? (*verbout) msg, fflush (stdout) : 0))
# define	MCHK(xxx)	((void) ((xxx) && ((xxx) -> magic != MAGIC) ? fprintf (stderr, "Invalid magic: expect %ld got %ld in %s:%d\n", MAGIC, (xxx) -> magic, __FILE__, __LINE__) : 0))
# define	MKMAGIC(ch1,ch2,ch3,ch4)					\
				((long) ((((unsigned char) (ch1)) << 24) |	\
					 (((unsigned char) (ch2)) << 16) |	\
					 (((unsigned char) (ch3)) << 8) |	\
					 ((unsigned char) (ch4))))
# else		/* NDEBUG */
# define	V(lvl,msg)
# define	MCHK(xxx)
# endif		/* NDEBUG */

# define	NO_ERR		0
# define	ERR_FAIL	(-1)
# define	ERR_FATAL	(-2)
# define	ERR_ABORT	(-3)

# define	ECONT(xxx)	(((xxx) == NO_ERR) || ((xxx) == ERR_FAIL))
# define	ESTOP(xxx)	(((xxx) == ERR_FATAL) || ((xxx) == ERR_ABORT))

# define	LGS_SENT	'+'
# define	LGF_SENT	'-'
# define	LGS_INF		'*'
# define	LGF_INF		'/'

# define	LG_INF		'i'
# define	LG_COST		'c'
# define	LG_SSESSION	's'
# define	LG_ESESSION	'e'
# define	LG_PROTO	'p'
/*}}}*/
/*{{{	typedefs */
typedef enum {
	False = 0,
	True = ! False
}	Bool;

typedef enum {
	Unknown,
	Ascii,
	Script,
	Tap,
	Ucp
}	Protocol;

typedef unsigned char	char_t;

typedef struct {
	char_t	*str;		/* the string itself			*/
	int	len;		/* the current length			*/
	int	size;		/* the allocated size			*/
}	string_t;

typedef struct {
	int	day, mon, year;
	int	hour, min, sec;
}	date_t;
/*}}}*/
/*{{{	prototypes */
/*{{{	utility */
extern char	*skip (char *str);
extern char	*skipch (char *str, char ch);
extern char	*getline (FILE *fp, Bool cont);
extern int	verbose_out (char *, ...);
/*}}}*/
/*{{{	string handling */
extern string_t	*snewc (char *str);
extern string_t	*snew (char_t *str, int len);
extern Bool	sexpand (string_t *s, int nsize);
extern Bool	scopy (string_t *dst, string_t *src);
extern Bool	scat (string_t *dst, string_t *src);
extern Bool	scopyc (string_t *dst, char *src);
extern Bool	scatc (string_t *dst, char *src);
extern string_t	*scut (string_t *str, int start, int len);
extern void	sdel (string_t *str, int start, int len);
extern Bool	sput (string_t *str, string_t *ins, int pos, int len);
extern Bool	sputc (string_t *str, char *ins, int pos, int len);
extern char	*sextract (string_t *s);
extern char	*schar (string_t *s);
extern void	*sfree (string_t *s);
extern void	srelease (string_t *s);
extern Bool	siscntrl (string_t *s, int pos);
extern Bool	sisspace (string_t *s, int pos);
extern Bool	sisdigit (string_t *s, int pos);
extern int	stoi (string_t *s);
/*}}}*/
/*{{{	date handling */
extern date_t	*dat_free (date_t *d);
extern date_t	*dat_parse (char *str);
extern int	dat_diff (date_t *d1, date_t *d2);
extern void	dat_clear (date_t *d);
extern void	dat_localtime (date_t *d);
/*}}}*/
/*{{{	tty handling */
extern void	*tty_open (char *dev, char *lckprefix, char *lckmethod);
extern void	*tty_close (void *sp);
extern Bool	tty_reopen (void *s, int msec);
extern void	tty_hangup (void *sp, int msec);
extern int	tty_fd (void *sp);
extern int	tty_setup (void *sp, Bool raw, Bool modem, int speed, int bpb, int sb, char parity);
extern void	tty_set_line_callback (void *sp, void (*func) (void *, string_t *, char_t, void *), char *sep, void *data);
extern void	tty_suspend_callback (void *sp, Bool susp);
extern int	tty_send (void *sp, char *str, int len);
extern int	tty_send_string (void *sp, char *str);
extern int	tty_expect (void *sp, int tout, ...);
extern int	tty_expect_list (void *sp, int tout, char **strs, int *lens);
extern int	tty_expect_string (void *sp, int tout, char *str);
extern int	tty_send_expect (void *sp, int deftout, char *str, char **opts);
extern void	tty_mdrain (void *sp, int msecs);
extern void	tty_drain (void *sp, int secs);
/*}}}*/
/*{{{	configuration */
extern void	*cfg_new (char *sep);
extern void	*cfg_read (char *fname, void *bp, char *sep);
extern void	*cfg_end (void *bp);
extern void	cfg_modify (void *bp, char *bname, char *var, char *val);
extern char	*cfg_get (void *bp, char *bname, char *var, char *dflt);
extern int	cfg_iget (void *bp, char *bname, char *var, int dflt);
extern Bool	cfg_bget (void *bp, char *bname, char *var, Bool dflt);
extern char	*cfg_block_get (void *bp, char *bname, char *var, char *dflt);
extern int	cfg_block_iget (void *bp, char *bname, char *var, int dflt);
extern Bool	cfg_block_bget (void *bp, char *bname, char *var, Bool dflt);
/*}}}*/
/*{{{	converting */
extern void	*cv_new (void);
extern void	*cv_free (void *cv);
extern void	*cv_reverse (void *src);
extern void	cv_define (void *cv, char_t src, char_t dst);
extern void	cv_sdefine (void *cv, char *src, char *dst);
extern void	cv_undefine (void *cv, char_t ch);
extern void	cv_sundefine (void *cv, char *ch);
extern void	cv_invalid (void *cv, char_t ch);
extern void	cv_sinvalid (void *cv, char *ch);
extern int	cv_read_table (void *cv, char *fname);
extern int	cv_write_table (void *cv, char *fname);
extern void	cv_merge (void *cv, void *in, Bool second);
extern int	cv_conv (void *cv, char_t ch);
/*}}}*/
/*{{{	ASCII protocol */
extern int	asc_login (void *ap, string_t *callid);
extern int	asc_logout (void *ap);
extern int	asc_transmit (void *ap, char *pid, char *msg);
extern int	asc_next (void *ap);
extern int	asc_sync (void *ap);
extern void	asc_config (void *ap, void (*logger) (char, char *, ...),
			    int deftout, char *alogin, char *alogout, char *apid, char *amsg, char *anext, char *async,
			    date_t *delay, date_t *expire, Bool rds);
extern void	asc_set_convtable (void *ap, void *ctab);
extern void	asc_add_convtable (void *ap, void *ctab);
extern void	*asc_new (void *sp);
extern void	*asc_free (void *ap);
extern int	asc_preinit (void);
extern void	asc_postdeinit (void);
/*}}}*/
/*{{{	scripting protocol */
extern int	scr_execute (void *sp, char *label, char *parm);
extern int	scr_load_string (void *sp, char *scr);
extern int	scr_load_file (void *sp, char *fname);
extern void	scr_config (void *sp, void (*logger) (char, char *, ...), date_t *delay, date_t *expire, Bool rds);
extern void	scr_set_convtable (void *sp, void *ctab);
extern void	scr_add_convtable (void *sp, void *ctab);
extern void	*scr_new (void *sp, char *typ, char *libdir);
extern void	*scr_free (void *sp);
extern int	scr_preinit (char *libdir);
extern void	scr_postdeinit (void);
/*}}}*/
/*{{{	Telocator Alphanumeric Protocol */
extern int	tap_login (void *tp, char *stype, char ttype, char *passwd, string_t *callid);
extern int	tap_logout (void *tp);
extern int	tap_transmit (void *tp, string_t **field, Bool last);
extern void	tap_config (void *tp, void (*logger) (char, char *, ...), Bool pre16);
extern void	tap_timeouts (void *tp, int t1, int t2, int t3, int t4, int t5);
extern void	tap_retries (void *tp, int n1, int n2, int n3, int licnt, int locnt);
extern void	tap_set_convtable (void *tp, void *ctab);
extern void	tap_add_convtable (void *tp, void *ctab);
extern void	*tap_new (void *sp);
extern void	*tap_free (void *tp);
extern int	tap_preinit (void);
extern void	tap_postdeinit (void);
/*}}}*/
/*{{{	Universal Computer Protocol */
extern int	ucp_login (void *up, string_t *callid);
extern int	ucp_logout (void *up);
extern int	ucp_transmit (void *up, string_t *pagerid, string_t *msg, Bool last);
extern void	ucp_config (void *up, void (*logger) (char, char *, ...),
			    Bool xtend, int stout, int retry, int rtout,
			    date_t *delay, date_t *expire, Bool rds);
extern void	ucp_set_convtable (void *up, void *ctab);
extern void	ucp_add_convtable (void *up, void *ctab);
extern void	*ucp_new (void *sp);
extern void	*ucp_free (void *up);
extern int	ucp_preinit (void);
extern void	ucp_postdeinit (void);
/*}}}*/
/*}}}*/
/*{{{	global variables */
extern int	verbose;
extern int	(*verbout) (char *, ...);
/*}}}*/
# endif		/* __PAGER_H */
