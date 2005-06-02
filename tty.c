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
	/* CAPI */
	int		is_capi;
	int		controller;
	char		*msn;
	int		connected;
}	serial;

typedef struct _expect {
	int	idx;
	char	*str;
	int	pos;
	int	len;
	struct _expect
		*next;
}	expect;

static int data_ready (serial *s, int fd, int *msec);

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
		s -> is_capi = 0;
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
	if (s -> is_capi) {
		return(hyla_capi_close(sp));
	}
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
	
	if (s -> is_capi) {
		return((s -> connected) ? True : False);
	}
	
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
		if (s -> is_capi) {
			hyla_capi_disconnect(sp);
			return;
		} else {
			tmp = s -> tty;
			cfsetispeed (& tmp, B0);
			cfsetospeed (& tmp, B0);
			if (tcsetattr (s -> fd, TCSANOW, & tmp) != -1) {
				msleep (msec);
				tcsetattr (s -> fd, TCSANOW, & s -> tty);
			}
			tty_reopen (s, msec);
		}
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
	while (len > 0) {
		if (s -> is_capi) {
			if (hyla_capi_send(sp, str, len)) {
				sent = len;
				len = 0;
			}
			break;
		}
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
	}
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

static size_t dev_read(serial *s, int fd, void *buf, size_t count)
{
	if (s -> is_capi)
		return(hyla_capi_read(s, buf, count));

	return(read(fd, buf, count));
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
		if ((n = data_ready (s, s -> fd, & msec)) > 0) {
			while ((n = dev_read (s, s -> fd, & ch, 1)) == 1) {
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
					if (s -> is_capi)
						hyla_capi_sendbreak(s, mult);
					else
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
					if (s -> is_capi)
						hyla_capi_drain(s);
					else
						tcdrain (s -> fd);
				}
				break;
			case '<':
				if (s && (s -> fd != -1)) {
					V (2, (" Iflush"));
					if (s -> is_capi)
						hyla_capi_iflush(s);
					else
						tcflush (s -> fd, TCIFLUSH);
				}
				break;
			case '>':
				if (s && (s -> fd != -1)) {
					V (2, (" Oflush"));
					if (s -> is_capi)
						hyla_capi_oflush(s);
					else
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
			if ((n = data_ready (s, s -> fd, & msecs)) > 0) {
				while ((m = dev_read (s, s -> fd, & ch, 1)) == 1) {
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

/*
 * CAPI 
 */

#include "capiconn.h"

static capiconn_context *ctx;
static unsigned applid;
static capi_contrinfo cinfo = { 0 , 0, 0 };
static struct capi_connection *capi_conn = NULL;
static serial *capi_serial;
static int conn_in_progress = 0;
static int capi_data_sent;
static unsigned char capi_data[4096];
static int capi_data_len = 0;

static void handle_messages(void)
{
	unsigned char *msg = 0;
	struct timeval tv;
	tv.tv_sec  = 1;
	tv.tv_usec = 0;
again:
	if (capi20_waitformessage(applid, &tv) == 0) {
		if (capi20_get_message(applid, &msg) == 0)
			capiconn_inject(applid, msg);
		tv.tv_sec  = 0;
		tv.tv_usec = 0;
		goto again;
	}
}
 
int device_is_capi(char *dev)
{
	if (!dev)
		return(0);

	if (strlen(dev) < 4)
		return(0);

	if (strncasecmp(dev, "capi", 4))
		return(0);

	return(1);
}

int hyla_is_capi(void *sp)
{
	serial	*s = (serial *) sp;
	return(s->is_capi);
}

static void capi_log(const char *format, ...)
{
	char buffer[2048];
	va_list ap;

	va_start(ap, format);
	vsprintf(buffer, format, ap);
	strcat(buffer, "\n");
	va_end(ap);
	V (3, (buffer));
}

static void chargeinfo(capi_connection *cp, unsigned long charge, int inunits)
{
	if (inunits) {
		V (3, ("charge in units: %d\n", charge));
	} else {
		V (3, ("charge in currency: %d\n", charge));
	}
}

static void put_message(unsigned appid, unsigned char *msg)
{
	unsigned err;
	err = capi20_put_message (appid, msg);
	if (err)
		V (2, ("capi putmessage %s 0x%x\n", 
			capi_info2str(err), err));
}

static void connected(capi_connection *cp, _cstruct NCPI)
{
	capi_serial -> connected = 1;
	conn_in_progress = 0;
	V (3, ("CAPI connected\n"));
}

static void disconnected(capi_connection *cp,
			int localdisconnect,
			unsigned reason,
			unsigned reason_b3)
{
	capi_serial -> connected = 0;
	conn_in_progress = 0;
	capi_conn = NULL;
	V (3, ("CAPI disconnected\n"));
}

static void handle_capi_sent(capi_connection *con, unsigned char* data)
{
	capi_data_sent = 1;
}

static void handle_capi_data(capi_connection *con, unsigned char* data, unsigned int len)
{
	int mlen = ((sizeof(capi_data) - capi_data_len) < len) ? 
			(sizeof(capi_data) - capi_data_len) : len;

	if (mlen) {
		memcpy(&capi_data[capi_data_len], data, mlen);
		capi_data_len += mlen;
	} else {
		V (3, ("\ncapi data buffer overflow\n"));
	}
}

capiconn_callbacks callbacks = {
	malloc: malloc,
	free: free,

	disconnected: disconnected,
	incoming: 0,
	connected: connected,
	received: handle_capi_data,
	datasent: handle_capi_sent,
	chargeinfo: chargeinfo,
	capi_put_message: put_message,
	debugmsg: (void (*)(const char *, ...))capi_log,
	infomsg: (void (*)(const char *, ...))capi_log,
	errmsg: (void (*)(const char *, ...))capi_log
};

void *hyla_capi_init(char *dev)
{
	serial	*s;
	char *p, *p1, *p2;
	
	if (s = (serial *) malloc (sizeof (serial))) {
		if ((capi20_register (1, 8, 2048, &applid)) != 0) {
			free (s);
			V (3, ("Error CAPI register\n"));
			return(NULL);
		}

# ifndef	NDEBUG
		s -> magic = MAGIC;
# endif		/* NDEBUG */

		s -> device = strdup (dev);
		p = skipch(s -> device, '/');
		p1 = p;
		p2 = skipch(p, '/');
		if (*p1)
			s -> controller = strtol(p1, NULL, 10);
		else
			s -> controller = 1;
		if (*p2)
			s -> msn = strdup(p2);
		else
			s -> msn = NULL;
	
		V (3, ("CAPI controller=%d MSN=%s\n", s -> controller, s -> msn));
		
		if ((ctx = capiconn_getcontext(applid, &callbacks)) != 0) {
			if (capiconn_addcontr(ctx, s -> controller, &cinfo) != CAPICONN_OK) {
				(void)capiconn_freecontext(ctx);
				(void)capi20_release(applid);
				if (s -> msn)
					free (s -> msn);
				if (s -> device)
					free (s -> device);
				free (s);
				s = NULL;
			}
			s -> fd = capi20_fileno(applid);
			s -> is_capi = 1;
			s -> connected = 0;
			s -> line = NULL;
			s -> sep = NULL;
			s -> callback = NULL;
			s -> data = NULL;
			s -> suspend = False;
			capi_serial = s;
		} else {
			(void)capi20_release(applid);
			V (3, ("Error CAPI-conn register\n"));
			if (s -> msn)
				free (s -> msn);
			if (s -> device)
				free (s -> device);
			free (s);
			s = NULL;
		}
	}
	return s;
}

void hyla_capi_disconnect(void *sp)
{
	serial	*s = (serial *) sp;
	time_t t;

	if (!s || !s -> connected)
		return;

	(void)capiconn_disconnect(capi_conn, 0);

	t = time(NULL) + 3; /* 3 seconds */
	do {
		handle_messages();
	} while ((time(NULL) < t) && (s -> connected));
}

void *hyla_capi_close(void *sp)
{
	serial	*s = (serial *) sp;

	MCHK (s);
	if (s) {
		hyla_capi_disconnect(sp);

		(void)capi20_release(applid);
		(void)capiconn_freecontext(ctx);

		if (s -> device)
			free (s -> device);
		if (s -> msn)
			free (s -> msn);
		if (s -> sep)
			free (s -> sep);
		if (s -> line)
			sfree (s -> line);
		free (s);
		capi_serial = NULL;
		capi_conn = NULL;
	}
	return(NULL);
}

int hyla_capi_connect(void *sp, char *dial, char *number)
{
	serial	*s = (serial *) sp;
	struct capi_connection *cp;
	time_t t;
	char num[128];

	conn_in_progress = 1;

	if (!dial)
		dial = "";
		
	sprintf(num, "%s%s", dial, number);

	/* X.75 connect */
	cp = capiconn_connect(ctx,
			s -> controller,
			2,	/* cip */
			num,
			s -> msn,
			0, 0, 0,
			0, 0, 0,
			0, 0);

	capi_conn = cp;

	t = time(NULL) + 10; /* 10 seconds */
	do {
		handle_messages();
	} while ((time(NULL) < t) && (conn_in_progress));

	if ((!s -> connected) && (capi_conn))
		(void)capiconn_disconnect(capi_conn, 0);

	return(s -> connected);
}

int hyla_capi_send(void *sp, char *str, int len)
{
	serial	*s = (serial *) sp;
	time_t t;

	if (!s || !s -> connected)
		return(0);

	capi_data_sent = 0;

	capiconn_send(capi_conn, str, len);

	t = time(NULL) + 5; /* 5 seconds */
	do {
		handle_messages();
	} while ((time(NULL) < t) && (!capi_data_sent));
	
	return(capi_data_sent);
}

size_t hyla_capi_read(void *sp, void *buf, size_t count)
{
	serial	*s = (serial *) sp;
	size_t ret=0;

	if (!capi_data_len)
		handle_messages();

	if (!s || !s -> connected)
		return(-1);
		
	ret = (count < capi_data_len) ? count : capi_data_len;
	
	memcpy(buf, capi_data, ret);

	capi_data_len -= ret;

	memmove(capi_data, capi_data+ret, capi_data_len);

	return(ret);
}

void hyla_capi_iflush(void *sp)
{
	capi_data_len = 0;
}

void hyla_capi_oflush(void *sp)
{
	/* not necessary */
}

void hyla_capi_drain(void *sp)
{
	/* not necessary */
}

void hyla_capi_sendbreak(void *sp, int duration)
{
	/* ... */
}

static int
data_ready (serial *s, int fd, int *msec)
{
	int		ret = 0;
	struct timeval	tv;
	fd_set		fset;

	if (s -> is_capi) {
		if (capi_data_len)
			return(1);
		if (!s -> connected)
			return(0);
	}
	
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
