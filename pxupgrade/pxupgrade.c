#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/getargs.h>

EXPORT	int	main		__PR((int ac, char **av));

int
main(ac, av)
	int	ac;
	char	*av[];
{
		int	cac = ac; 
		char	* const *cav = av; 
	BOOL	prversion = FALSE;

	getallargs(&cac, &cav, "version", &prversion);
	if (prversion)
		printf("| PXUpdate V1.00-dummy\n");
	printf("\nThis is a dummy pxupgrade program.\n");
	printf("The original program is not OpenSource and cannot be published\n");
	printf("as it is under NDA wit Plextor.\n");
	printf("If you need a working pxupgrade binary, install a packet from www.blastwave.org\n");
	printf("or download a binary from ftp://ftp.berlios.de/pub/cdrecord/firmware/plextor/\n\n");
	return (0);
}
