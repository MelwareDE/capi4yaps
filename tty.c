/*	-*- mode: c; mode: fold -*-	*/
/*{{{	includes */
# include	"config.h"
# include	<stdio.h>
# include	<stdlib.h>
# include	<stdarg.h>
# include	<ctype.h>
# include	<unistd.h>
# include	<fcntl.h>
# include	<string.h>
# include	<termios.h>
# include	<errno.h>
# include	<signal.h>
# include	<sys/time.h>
# include	<sys/types.h>
# include	<sys/stat.h>
# if		HAVE_SYS_SELECT_H
# include	<sys/select.h>
# endif		/* HAVE_SYS_SELECT_H */
# if		HAVE_SYS_SYSMACROS_H
# include	<sys/sysmacros.h>
# elif		HAVE_SYS_MKDEV_H
# include	<sys/mkdev.h>
# else		/* ! HAVE_SYS_SYSMACROS_H && ! HAVE_SYS_MKDEV_H */
# define	major(xx)	(((xx) >> 8) & 0xff)
# define	minor(xx)	((xx) & 0xff)
# endif		/* HAVE_SYS_SYSMACROS_H || HAVE_SYS_MKDEV_H */
# include	"pager.h"
/*}}}*/
/*{{{	statics and typedefs */
static struct {
	int	speed;
	speed_t	tok;
}	stab[] = {
	{	   300,	B300	},
	{	  1200,	B1200	},
	{	  2400,	B2400	},
	{	  4800,	B4800	},
	{	  9600,	B9600	},
	{	 19200,	B19200	},
	{	 38400,	B38400	},
# ifdef		B57600
	{	 57600,	B57600	},
# endif		/* B57600 */
# ifdef		B115200
	{	115200, B115200	},
# endif		/* B115200 */
# ifdef		B230400
	{	230400, B230400	},
# endif		/* B230400 */
# ifdef		B460800
	{	460800, B460800	},
# endif		/* B460800 */
	{	-1,	B9600	}
};

static char	*lckmth[] = {
	"ascii",
	"binary",
	"lower",
	"upper",
	"sysv4",
	"timeout",
	NULL
};

typedef struct {
# ifndef	NDEBUG
# define	MAGIC		MKMAGIC ('t', 't', 'y', '\0')
	long		magic;
# endif		/* NDEBUG */
	char		*lck;
	char		*device;
	struct termios	tty, sav;
	int		fd;
	string_t	*line;
	char		*sep;
	void		(*callback) (void *, string_t *, char_t, void *);
	void		*data;
	Bool		suspend;
}	serial;

typedef struct _expect {
	int	idx;
	char	*str;
	int	pos;
	int	len;
	struct _expect
		*next;
}	expect;
/*}}}*/
/*{{{	support routines */
static char *
mkprint (char *str, int len)
{
	static char	*buf = NULL;
	static int	size = 0;
	int		n;
	char		ch;
	char		extra;
	char		*ptr;

	if (len >= size) {
		size = len + 128;
		if (! (buf = Realloc (buf, size + 2)))
			return NULL;
	}
	extra = '\0';
	for (n = 0; len > 0; ++str, --len) {
		if (n + 8 >= size) {
			size += 128;
			if (! (buf = Realloc (buf, size + 2)))
				break;
		}
		ch = *str;
		if (ch & 0x80) {
			buf[n++] = '<';
			buf[n++] = 'M';
			buf[n++] = '-';
			ch &= 0x7f;
			extra = '>';
		}
		switch (ch) {
		case '\x00':	ptr = "<nul>";	break;
		case '\x01':	ptr = "<soh>";	break;
		case '\x02':	ptr = "<stx>";	break;
		case '\x03':	ptr = "<etx>";	break;
		case '\x04':	ptr = "<eot>";	break;
		case '\x05':	ptr = "<enq>";	break;
		case '\x06':	ptr = "<ack>";	break;
		case '\x07':	ptr = "<bel>";	break;
		case '\x08':	ptr = "<bs>";	break;
		case '\x09':	ptr = "<ht>";	break;
		case '\x0a':	ptr = "<lf>";	break;
		case '\x0b':	ptr = "<vt>";	break;
		case '\x0c':	ptr = "<ff>";	break;
		case '\x0d':	ptr = "<cr>";	break;
		case '\x0e':	ptr = "<so>";	break;
		case '\x0f':	ptr = "<si>";	break;
		case '\x10':	ptr = "<dle>";	break;
		case '\x11':	ptr = "<dc1>";	break;
		case '\x12':	ptr = "<dc2>";	break;
		case '\x13':	ptr = "<dc3>";	break;
		case '\x14':	ptr = "<dc4>";	break;
		case '\x15':	ptr = "<nak>";	break;
		case '\x16':	ptr = "<syn>";	break;
		case '\x17':	ptr = "<etb>";	break;
		case '\x18':	ptr = "<can>";	break;
		case '\x19':	ptr = "<em>";	break;
		case '\x1a':	ptr = "<sub>";	break;
		case '\x1b':	ptr = "<esc>";	break;
		case '\x1c':	ptr = "<fs>";	break;
		case '\x1d':	ptr = "<gs>";	break;
		case '\x1e':	ptr = "<rs>";	break;
		case '\x1f':	ptr = "<us>";	break;
		case '\x7f':	ptr = "<del>";	break;
		default:
			ptr = NULL;
			buf[n++] = ch;
			break;
		}
		if (ptr)
			while (*ptr)
				buf[n++] = *ptr++;
		if (extra) {
			buf[n++] = extra;
			extra = '\0';
		}
	}
	if (buf)
		buf[n] = '\0';
	return buf;
}

static inline void
msleep (int msec)
{
	struct timeval	tv;

	if (msec > 0) {
		do {
			tv.tv_sec = msec / 1000;
			tv.tv_usec = (msec % 1000) * 1000;
			errno = 0;
		}	while ((select (0, NULL, NULL, NULL, & tv) < 0) && (errno == EINTR));
	}
}

static inline int
data_ready (int fd, int *msec)
{
	int		ret = 0;
	struct timeval	tv;
	fd_set		fset;
	
	FD_ZERO (& fset);
	FD_SET (fd, & fset);
	tv.tv_sec = *msec / 1000;
	tv.tv_usec = (*msec % 1000) * 1000;
	if (((ret = select (fd + 1, & fset, NULL, NULL, & tv)) > 0) && FD_ISSET (fd, & fset)) {
		*msec = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
		return 1;
	}
	return ret;
}

static Bool
do_lock (serial *s, char *dev, char *prefix, char *method)
{
	Bool		binary;
	Bool		lower, upper;
	Bool		sysv4;
	int		tout;
	struct stat	st;
	char		*ptr, *sav, *val;
	int		len;
	char		*bdev;
	int		fd;
	char		buf[32];
	int		n, m;
	pid_t		pid;

	s -> lck = NULL;
	if (prefix) {
		binary = False;
		lower = False;
		upper = False;
		sysv4 = False;
		tout = 0;
		if (method && (method = strdup (method))) {
			for (ptr = method; *ptr; ) {
				sav = ptr;
				ptr = skipch (ptr, ',');
				val = skipch (sav, '=');
				len = strlen (sav);
				for (n = 0; lckmth[n]; ++n)
					if (! strncmp (lckmth[n], sav, len))
						break;
				switch (n) {
				case 0:		/* ascii */
					binary = False;
					break;
				case 1:		/* binary */
					binary = True;
					break;
				case 2:		/* lower */
					lower = True;
					upper = False;
					break;
				case 3:		/* upper */
					lower = False;
					upper = True;
					break;
				case 4:		/* sysv4 */
					sysv4 = True;
					break;
				case 5:		/* timeout */
					tout = atoi (val);
					break;
				}
			}
			free (method);
		}
		if (sysv4) {
			bdev = NULL;
			if ((stat (dev, & st) != -1) && S_ISCHR (st.st_mode) && (bdev = malloc (96)))
				sprintf (bdev, "%03d.%03d.%03d", major (st.st_dev), major (st.st_rdev), minor (st.st_rdev));
		} else {
			if (bdev = strrchr (dev, '/'))
				++bdev;
			else
				bdev = dev;
			bdev = strdup (bdev);
		}
		len = strlen (prefix);
		if (bdev && (s -> lck = malloc (len + strlen (bdev) + 4))) {
			sprintf (s -> lck, "%s%s", prefix, bdev);
			free (bdev);
			if (upper || lower) {
				ptr = s -> lck + len;
				while (*ptr) {
					if (lower)
						*ptr = tolower (*ptr);
					else if (upper)
						*ptr = toupper (*ptr);
					++ptr;
				}
			}
			do {
				for (n = 0; n < 2; ++n) {
					if ((fd = open (s -> lck, O_CREAT | O_EXCL | O_WRONLY, 0600)) != -1)
						break;
					if ((! n) && ((fd = open (s -> lck, O_RDONLY)) != -1)) {
						pid = 0;
						if (binary) {
							if (read (fd, & pid, sizeof (pid)) != sizeof (pid))
								pid = 0;
						} else {
							if ((m = read (fd, buf, sizeof (buf) - 1)) > 1) {
								buf[m - 1] = '\0';
								pid = (int) atoi (buf);
							}
						}
						close (fd);
						fd = -1;
						if ((pid > 0) && (kill (pid, 0) < 0) && (errno == ESRCH))
							unlink (s -> lck);
						else
							break;
					}
				}
				if ((fd < 0) && (tout > 0))
					sleep (1);
			}	while ((fd < 0) && (tout-- > 0));
			if (fd != -1) {
				pid = getpid ();
				if (binary)
					write (fd, & pid, sizeof (pid));
				else {
					sprintf (buf, "%10d\n", (int) pid);
					write (fd, buf, strlen (buf));
				}
# if		HAVE_FCHMOD				
				fchmod (fd, 0644);
# else		/* HAVE_FCHMOD */
				chmod (s -> lck, 0644);
# endif		/* HAVE_FCHMOD */
# if		HAVE_FCHOWN				
				fchown (fd, geteuid (), getegid ());
# else		/* HAVE_FCHOWN */
				chown (s -> lck, geteuid (), getegid ());
# endif		/* HAVE_FCHOWN */
				close (fd);
			} else {
				free (s -> lck);
				s -> lck = NULL;
				return False;
			}
		} else {
			if (bdev)
				free (bdev);
			return False;
		}
	}
	return True;
}

static void
do_unlock (serial *s)
{
	if (s -> lck) {
		unlink (s -> lck);
		free (s -> lck);
		s -> lck = NULL;
	}
}
/*}}}*/
/*{{{	open/close/reopen */
void *
tty_open (char *dev, char *lckprefix, char *lckmethod)
{
	serial	*s;
	int	n;
	
	if (s = (serial *) malloc (sizeof (serial))) {
# ifndef	NDEBUG
		s -> magic = MAGIC;
# endif		/* NDEBUG */
		if (do_lock (s, dev, lckprefix, lckmethod)) {
			if ((s -> fd = open (dev, O_RDWR)) != -1) {
				n = tcgetattr (s -> fd, & s -> sav);
				if ((n < 0) || (! (s -> device = strdup (dev)))) {
					close (s -> fd);
					do_unlock (s);
					free (s);
					s = NULL;
				} else {
					s -> tty = s -> sav;
					s -> line = NULL;
					s -> sep = NULL;
					s -> callback = NULL;
					s -> data = NULL;
					s -> suspend = False;
				}
			} else {
				do_unlock (s);
				free (s);
				s = NULL;
			}
		} else {
			free (s);
			s = NULL;
		}
	}
	return s;
}

void *
tty_close (void *sp)
{
	serial	*s = (serial *) sp;

	MCHK (s);
	if (s) {
		if (s -> fd != -1) {
			tcsetattr (s -> fd, TCSANOW, & s -> sav);
			close (s -> fd);
		}
		do_unlock (s);
		if (s -> device)
			free (s -> device);
		if (s -> sep)
			free (s -> sep);
		if (s -> line)
			sfree (s -> line);
		free (s);
	}
	return NULL;
}

Bool
tty_reopen (void *sp, int msec)
{
	serial	*s = (serial *) sp;

	MCHK (s);
	if (s -> fd != -1) {
		close (s -> fd);
		if (msec > 0)
			msleep (msec);
		s -> fd = -1;
	}
	if (s -> device && ((s -> fd = open (s -> device, O_RDWR)) != -1))
		tcsetattr (s -> fd, TCSANOW, & s -> tty);
	return s -> fd != -1 ? True : False;
}
/*}}}*/
/*{{{	hangup, get fd */
void
tty_hangup (void *sp, int msec)
{
	serial		*s = (serial *) sp;
	struct termios	tmp;

	MCHK (s);
	V (2, ("[Hangup] "));
	if (s && (s -> fd != -1)) {
		tmp = s -> tty;
		cfsetispeed (& tmp, B0);
		cfsetospeed (& tmp, B0);
		if (tcsetattr (s -> fd, TCSANOW, & tmp) != -1) {
			msleep (msec);
			tcsetattr (s -> fd, TCSANOW, & s -> tty);
		}
		tty_reopen (s, msec);
	}
	V (2, ("\n"));
}

int
tty_fd (void *sp)
{
	serial	*s = (serial *) sp;
	
	MCHK (s);
	return s ? s -> fd : -1;
}
/*}}}*/
/*{{{	configuration */
int
tty_setup (void *sp, Bool raw, Bool modem, int speed, int bpb, int sb, char par)
{
	serial	*s = (serial *) sp;
	int	n;

	MCHK (s);
	if ((! s) || (s -> fd < 0))
		return -1;
	if ((bpb < 5) || (bpb > 8) ||
	    ((sb != 1) && (sb != 2)) ||
	    ((par != 'n') && (par != 'e') && (par != 'o')))
		return -1;
	for (n = 0; stab[n].speed > 0; ++n)
		if (stab[n].speed == speed)
			break;
	if (stab[n].speed < 0)
		return -1;
	if (raw) {
		s -> tty.c_iflag &= ~(IGNCR | ICRNL | INLCR | ISTRIP | IXON | IXOFF);
		s -> tty.c_iflag |= IGNBRK;
		s -> tty.c_oflag = 0;
		s -> tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG);
# ifdef		CRTSCTS
# ifdef		sun
		/* SunOS 4.x needs RTSCTS off if carrier detect is on */
		/* but there is no carrier present (fkk) */
		s -> tty.c_cflag &= ~CRTSCTS;
# else		/* sun */
		s -> tty.c_cflag |= CRTSCTS;
# endif		/* sun */
# endif		/* CRTSCTS */
		s -> tty.c_cc[VMIN] = 0;
		s -> tty.c_cc[VTIME] = 0;
	} else {
		s -> tty = s -> sav;
		s -> tty.c_lflag |= ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG;
	}
	if (modem) {
# if	0		
		s -> tty.c_cflag &= ~CLOCAL;
# else
		s -> tty.c_cflag |= CLOCAL;
# endif		
		s -> tty.c_cflag |= HUPCL;
	} else {
		s -> tty.c_cflag |= CLOCAL;
		s -> tty.c_cflag &= ~HUPCL;
	}
	cfsetispeed (& s -> tty, stab[n].tok);
	cfsetospeed (& s -> tty, stab[n].tok);
	s -> tty.c_cflag &= ~(CSIZE);
	s -> tty.c_cflag |= CREAD;
	switch (sb) {
	default:
	case 1:	s -> tty.c_cflag &= ~CSTOPB;	break;
	case 2:	s -> tty.c_cflag |= CSTOPB;	break;
	}
	switch (bpb) {
	case 5:	s -> tty.c_cflag |= CS5;	break;
	case 6:	s -> tty.c_cflag |= CS6;	break;
	case 7:	s -> tty.c_cflag |= CS7;	break;
	default:
	case 8:	s -> tty.c_cflag |= CS8;	break;
	}
	switch (par) {
	default:
	case 'n':
		s -> tty.c_cflag &= ~PARENB;
		break;
	case 'e':
		s -> tty.c_cflag &= ~PARODD;
		s -> tty.c_cflag |= PARENB;
		break;
	case 'o':
		s -> tty.c_cflag |= PARENB | PARODD;
		break;
	}
	return tcsetattr (s -> fd, TCSANOW, & s -> tty) < 0 ? -1 : 0;
}
/*}}}*/
/*{{{	callback */
void
tty_set_line_callback (void *sp, void (*func) (void *, string_t *, char_t, void *), char *sep, void *data)
{
	serial	*s = (serial *) sp;
	
	MCHK (s);
	if (s) {
		if (! (s -> callback = func)) {
			if (s -> line)
				s -> line = sfree (s -> line);
			if (s -> sep) {
				free (s -> sep);
				s -> sep = NULL;
			}
			s -> data = NULL;
		} else {
			if (s -> sep)
				free (s -> sep);
			s -> sep = sep ? strdup (sep) : NULL;
			s -> data = data;
		}
		s -> suspend = False;
	}
}

void
tty_suspend_callback (void *sp, Bool susp)
{
	serial	*s = (serial *) sp;
	
	MCHK (s);
	if (s)
		if (s -> suspend = susp)
			if (s -> line)
				s -> line -> len = 0;
}
/*}}}*/
/*{{{	sending */
int
tty_send (void *sp, char *str, int len)
{
	serial	*s = (serial *) sp;
	int	n, sent;
	
	MCHK (s);
	if ((! s) || (s -> fd < 0) || (! str))
		return -1;
	V (2, ("[Send] %s", mkprint (str, len)));
	sent = 0;
	while (len > 0)
		if ((n = write (s -> fd, str, len)) > 0) {
			str += n;
			sent += n;
			len -= n;
		} else if (! n)
			break;
		else if (errno == EIO) {
			if (! tty_reopen (s, 0))
				break;
		} else if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			msleep (200);
		else if (errno != EINTR)
			break;
	V (2, ("\n"));
	return sent;
}

int
tty_send_string (void *sp, char *str)
{
	return str ? tty_send (sp, str, strlen (str)) : -1;
}
/*}}}*/
/*{{{	expecting */
static void
addline (serial *s, char ch)
{
	if (s -> callback && s -> sep && (! s -> suspend) && ch) {
		if (! s -> line)
			s -> line = snew (NULL, 32);
		else if (s -> line -> len + 1 >= s -> line -> size)
			if (! sexpand (s -> line, s -> line -> size * 2))
				return;
		if (strchr (s -> sep, ch)) {
			(*s -> callback) ((void *) s, s -> line, (char_t) ch, s -> data);
			s -> line -> len = 0;
		} else
			s -> line -> str[s -> line -> len++] = (char_t) ch;
	}
}

static int
do_expect (serial *s, int tout, expect *base)
{
	int	ret;
	int	msec;
	int	n;
	expect	*run;
	char	ch;

	if ((! s) || (s -> fd < 0))
		return -1;
	V (2, ("[Expect] "));
	for (run = base; run; run = run -> next)
		run -> pos = 0;
	msec = (tout > 0) ? tout * 1000 : 0;
	ret = 0;
	while (! ret)
		if ((n = data_ready (s -> fd, & msec)) > 0) {
			while ((n = read (s -> fd, & ch, 1)) == 1) {
				addline (s, ch);
				V (3, ("%s", mkprint (& ch, 1)));
				for (run = base; run; run = run -> next)
					if (run -> str[run -> pos] == ch) {
						run -> pos++;
						if (run -> pos == run -> len) {
							ret = run -> idx;
							break;
						}
					} else
						run -> pos = 0;
				if (ret)
					break;
			}
			if (n < 0)
				if (errno == EIO)
					tty_reopen (s, 0);
		} else if (! n)
			break;
	if (verbose > 1)
		if (run)
			printf (" got %s", mkprint (run -> str, run -> len));
		else
			printf (" timeout");
	V (2, ("\n"));
	return ret;
}

int
tty_expect (void *sp, int tout, ...)
{
	va_list	par;
	int	ret;
	char	*ptr;
	expect	*base, *prev, *tmp;
	int	idx;

	va_start (par, tout);
	MCHK ((serial *) sp);
	ret = 0;
	base = NULL;
	prev = NULL;
	idx = 1;
	while (ptr = va_arg (par, char *))
		if (tmp = (expect *) malloc (sizeof (expect))) {
			tmp -> idx = idx++;
			tmp -> str = ptr;
			tmp -> pos = 0;
			tmp -> len = va_arg (par, int);
			tmp -> next = NULL;
			if (prev)
				prev -> next = tmp;
			else
				base = tmp;
			prev = tmp;
		} else
			break;
	if (! ptr)
		ret = do_expect ((serial *) sp, tout, base);
	else
		ret = -1;
	while (base) {
		tmp = base;
		base = base -> next;
		free (tmp);
	}
	va_end (par);
	return ret;
}

int
tty_expect_list (void *sp, int tout, char **strs, int *lens)
{
	int	n;
	int	ret;
	expect	*base, *prev, *tmp;
	
	MCHK ((serial *) sp);
	base = NULL;
	prev = NULL;
	for (n = 0; strs[n]; ++n)
		if (tmp = (expect *) malloc (sizeof (expect))) {
			tmp -> idx = n + 1;
			tmp -> str = strs[n];
			tmp -> pos = 0;
			tmp -> len = lens[n];
			tmp -> next = NULL;
			if (prev)
				prev -> next = tmp;
			else
				base = tmp;
			prev = tmp;
		} else
			break;
	if (strs[n])
		ret = -1;
	else
		ret = do_expect ((serial *) sp, tout, base);
	while (base) {
		tmp = base;
		base = base -> next;
		free (tmp);
	}
	return ret;
}

int
tty_expect_string (void *sp, int tout, char *str)
{
	expect	tmp;

	MCHK ((serial *) sp);
	if (! str)
		return -1;
	tmp.idx = 1;
	tmp.str = str;
	tmp.pos = 0;
	tmp.len = strlen (str);
	tmp.next = NULL;
	return do_expect ((serial *) sp, tout, & tmp);
}
/*}}}*/
/*{{{	send/expect */
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

static char *
expand (char *str, char **opts, int ocnt)
{
	char	*ret;
	int	len, siz;
	int	idx, olen;

	ret = NULL;
	len = 0;
	siz = 0;
	while (*str) {
		if (len >= siz) {
			siz += (siz ? siz : 32);
			if (! (ret = Realloc (ret, siz + 2)))
				break;
		}
		if (*str == '\\') {
			++str;
			if (*str) {
				switch (*str) {
				case 'a':	ret[len++] = '\a';	break;
				case 'b':	ret[len++] = '\b';	break;
				case 'f':	ret[len++] = '\f';	break;
				case 'l':	ret[len++] = '\012';	break;
				case 'n':	ret[len++] = '\n';	break;
				case 'r':	ret[len++] = '\r';	break;
				case 's':	ret[len++] = ' ';	break;
				case 't':	ret[len++] = '\t';	break;
				default:	ret[len++] = *str;	break;
				}
				++str;
			}
		} else if (*str == '^') {
			++str;
			if (*str) {
				if (*str == '?')
					ret[len++] = '\x7f';
				else
					ret[len++] = *str & 0x1f;
				++str;
			}
		} else if (*str == '%') {
			++str;
			idx = 0;
			while (isdigit (*str)) {
				idx *= 10;
				idx += tonum (*str++);
			}
			if ((idx >= 0) && (idx < ocnt)) {
				olen = strlen (opts[idx]);
				if (len + olen >= siz) {
					siz = len + olen + 64;
					if (! (ret = Realloc (ret, siz + 2)))
						break;
				}
				strcpy (ret + len, opts[idx]);
				len += olen;
			}
		} else if ((*str == '\'') || (*str == '"'))
			++str;
		else
			ret[len++] = *str++;
	}
	if (ret)
		ret[len] = '\0';
	return ret;
}

static int
handle_command (void *sp, char *str)
{
	serial	*s = (serial *) sp;
	char	*p1;
	int	mult;
	int	ret;
	
	ret = 0;
	V (2, ("[Cmd"));
	for (p1 = str; *p1; ) {
		if (isdigit (*p1)) {
			mult = 0;
			while (isdigit (*p1)) {
				mult *= 10;
				mult += tonum (*p1++);
			}
		} else
			mult = 1;
		if (*p1)
			switch (*p1++) {
			case 'd':
				V (2, (" Dzz %d", mult));
				sleep (mult);
				break;
			case 'D':
				V (2, (" Mdzz %d", mult));
				msleep (mult);
				break;
			case 'b':
				if (s && (s -> fd != -1)) {
					V (2, (" Brk %d", mult));
					tcsendbreak (s -> fd, mult);
				}
				break;
			case 'h':
				if (s && (s -> fd != -1)) {
					V (2, (" Hup"));
					tty_hangup (sp, mult * 500);
				}
				break;
			case 'o':
				if (s && (s -> fd != -1)) {
					V (2, (" Drain"));
					tcdrain (s -> fd);
				}
				break;
			case '<':
				if (s && (s -> fd != -1)) {
					V (2, (" Iflush"));
					tcflush (s -> fd, TCIFLUSH);
				}
				break;
			case '>':
				if (s && (s -> fd != -1)) {
					V (2, (" Oflush"));
					tcflush (s -> fd, TCOFLUSH);
				}
				break;
			case 'f':
				V (2, (" fail"));
				ret = -1;
				break;
			case 's':
				V (2, (" success"));
				ret = 1;
				break;
			}
		}
	V (2, ("]\n"));
	return ret;
}

static int
expect_expr (void *sp, int deftout, char *line)
{
	int	ret;
	char	**ex;
	int	cnt, siz;
	int	n, m, tout, slen;
	char	*ptr;
	char	*p1, *p2;
	char	**list;
	int	*len;

	MCHK ((serial *) sp);
	ex = NULL;
	cnt = 0;
	siz = 0;
	for (ptr = line; *ptr; ) {
		if (cnt >= siz) {
			siz += 4;
			if (! (ex = (char **) Realloc (ex, (siz + 1) * sizeof (char *))))
				break;
		}
		ex[cnt++] = ptr;
		while (*ptr && (*ptr != '-'))
			++ptr;
		if (*ptr)
			*ptr++ = '\0';
	}
	if (! ex)
		return -1;
	ret = 0;
	for (n = 0; n < cnt; ++n) {
		ptr = ex[n];
		if (isdigit (*ptr)) {
			tout = 0;
			while (isdigit (*ptr)) {
				tout *= 10;
				tout += tonum (*ptr++);
			}
		} else
			tout = deftout;
		p1 = ptr;
		for (siz = 1, p2 = p1; p2; p2 = strchr (p2, '|'))
			++siz, ++p2;
		if ((list = (char **) malloc ((siz + 1) * sizeof (char *))) &&
		    (len = (int *) malloc ((siz + 1) * sizeof (int)))) {
			for (n = 0, p1 = ptr; p1; ++n) {
				p2 = p1;
				if (p1 = strchr (p1, '|'))
					*p1++ = '\0';
				list[n] = p2;
				len[n] = strlen (p2);
			}
			list[n] = NULL;
			len[n] = 0;
			if (tty_expect_list (sp, tout, list, len) != 1)
				ret = -1;
			free (list);
			free (len);
		} else {
			ret = -1;
			break;
		}
		if ((ret < 0) && (n + 1 < cnt)) {
			++n;
			ptr = ex[n];
			ret = 0;
			if (*ptr == '!') {
				if (m = handle_command (sp, ptr + 1)) {
					if (m < 0)
						ret = -1;
					break;
				}
			} else {
				slen = strlen (ptr);
				if (tty_send (sp, ptr, slen) != slen) {
					ret = -1;
					break;
				}
			}
		} else
			n = cnt;
	}
	if (ex)
		free (ex);
	return ret;
}

int
tty_send_expect (void *sp, int deftout, char *str, char **opts)
{
	serial	*s = (serial *) sp;
	int	ret;
	int	ocnt;
	char	*sav, *ptr, *line;
	int	n;
	char	quote;
	Bool	esc;

	MCHK (s);
	ret = -1;
	if (opts)
		for (ocnt = 0; opts[ocnt]; ++ocnt)
			;
	else
		ocnt = 0;
	if (str = strdup (str)) {
		ret = 0;
		for (ptr = str; *ptr && (! ret); ) {
			sav = ptr;
			esc = False;
			quote = '\0';
			while (*ptr) {
				if (esc)
					esc = False;
				else if (*ptr == '\\')
					esc = True;
				else if (quote) {
					if (*ptr == quote)
						quote = '\0';
				} else if ((*ptr == '\'') || (*ptr == '"'))
					quote = *ptr;
				else if (isspace (*ptr))
					break;
				++ptr;
			}
			if (*ptr) {
				*ptr++ = '\0';
				while (isspace (*ptr))
					++ptr;
			}
			if (line = expand (sav, opts, ocnt)) {
				if (line[0] == '<') {
					if (expect_expr (sp, deftout, line + 1) < 0)
						ret = -1;
				} else if (line[0] == '!') {
					if ((n = handle_command (sp, line + 1)) < 0)
						ret = -1;
					else if (n > 0)
						while (*ptr)
							++ptr;
				} else {
					n = strlen (line);
					if (tty_send (sp, line, n) != n)
						ret = -1;
				}
				free (line);
			} else
				ret = -1;
		}
		free (str);
	}
	return ret;
}
/*}}}*/
/*{{{	draining */
void
tty_mdrain (void *sp, int msecs)
{
	serial	*s = (serial *) sp;
	int	n, m;
	char	ch;

	MCHK (s);
	if (s && (s -> fd != -1)) {
		V (2, ("[Drain] "));
		if (msecs < 0)
			msecs = 0;
		do {
			if ((n = data_ready (s -> fd, & msecs)) > 0) {
				while ((m = read (s -> fd, & ch, 1)) == 1) {
					addline (s, ch);
					V (2, ("%s", mkprint (& ch, 1)));
				}
				if (m < 0)
					if (errno == EIO)
						tty_reopen (s, 0);
			}
		}	while (n != 0);
		V (2, ("\n"));
	}
}

void
tty_drain (void *sp, int secs)
{
	tty_mdrain (sp, secs * 1000);
}
/*}}}*/
