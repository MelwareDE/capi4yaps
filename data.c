/*	-*- mode: c; mode: fold -*-	*/
# include	"config.h"
# include	"pager.h"

int	verbose = 0;
int	(*verbout) (char *, ...) = verbose_out;
