#
# Aplot
#   copyright 2001 by Amish S. Dave
#

PREFIX=/usr/local
MODE=655
OWNER=root
GROUP=root
INSTALL=/usr/bin/install -b -D -o ${OWNER} -g ${GROUP}

default: aplot

aplot:
	make -C src/ aplot

tar: 
	make -C src/ spotless
	make -C src/ release
	tar -C ../ --exclude aplot/misc --one-file-system -cpf ../aplot_`date +%Y%m%d`.tar aplot 
	bzip2 -9 ../aplot_`date +%Y%m%d`.tar
	mv -f ../aplot_`date +%Y%m%d`.tar.bz2 /var/www/html/software/

depend:
	make -C src/ depend

install: aplot 
	${INSTALL} -m ${MODE} src/aplot ${PREFIX}/bin/aplot

clean:
	make -C src/ clean

spotless:
	make -C src/ spotless

