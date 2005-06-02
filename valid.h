/*	-*- mode: c; mode: fold -*-	*/
# ifndef	__VALID_H
# define	__VALID_H		1
# if		HAVE_REGEX_H
# include	<regex.h>
# endif		/* HAVE_REGEX_H */

typedef struct {
# if		HAVE_REGEX_H
	regex_t	r;
# else		/* HAVE_REGEX_H */
	char	**pats;
	int	*lens;
	int	malways;
# endif		/* HAVE_REGEX_H */
}	valid_t;

extern valid_t	*v_new (char *pattern);
extern Bool	v_alidate (valid_t *v, char *str, int *start, int *end);
extern void	v_free (valid_t *v);
# endif		/* __VALID_H */
