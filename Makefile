#
# Aplot
#   copyright 2001 by Amish S. Dave
#

PREFIX=/usr/local

default: aplot

aplot:
	make -C src/ aplot

install: aplot 
	${INSTALL} -m ${MODE} src/aplot ${PREFIX}/bin/aplot

clean:
	make -C src/ clean

spotless:
	make -C src/ spotless

