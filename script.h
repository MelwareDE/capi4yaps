/*	-*- mode: c; mode: fold -*-	*/
# ifndef	__SCRIPT_H
# define	__SCRIPT_H		1
/*{{{	typedefs */
typedef struct script	script;

typedef struct {
	char	*typ;
	int	(*finit) (script *, char *);
	void	(*fdeinit) (script *);
	int	(*fexec) (script *, char *, char *);
	int	(*fsload) (script *, char *);
	int	(*ffload) (script *, char *);
	int	(*fpreinit) (char *);
	void	(*fpostdeinit) (void);
}	funcs;

struct script {
# ifndef	NDEBUG
# define	MAGIC		MKMAGIC ('s', 'c', 'r', '\0')
	long	magic;
# endif		/* NDEBUG */
	void	*sp;
	void	*ctab;
	void	(*logger) (char, char *, ...);
	date_t	delay;
	date_t	expire;
	Bool	rds;
	void	*priv;
	funcs	*f;
};
/*}}}*/

extern char	*scr_convert (script *s, char *str);
# ifdef		SCRIPT_SLANG
extern funcs	fslang;
# endif		/* SCRIPT_SLANG */
# ifdef		SCRIPT_LUA
extern funcs	flua;
# endif		/* SCRIPT_LUA */
# endif		/* __SCRIPT_H */
