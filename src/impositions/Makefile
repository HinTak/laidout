###############################################################
##############                                  ###############
#############  Laidout src/impositions Makefile  ##############
##############                                  ###############
###############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) -I. -I.. -I$(LAXDIR)/.. -I/usr/include/freetype2 -I$(POLYPTYCHBASEDIR) -I/usr/include/GraphicsMagick/


objs= \
	imposition.o \
	impositioneditor.o \
	singles.o \
	impositions.o \
	dodecahedron.o \
	box.o \
	netimposition.o \
	simplenet.o \
	signatures.o \
	signatureinterface.o \
	netdialog.o \
	polyptychwindow.o \
	singleseditor.o

#frompolyptych= glbase.o  gloverlay.o  hedronwindow.o  nets.o  poly.o  polyrender.o


impositions: $(objs)

#nets.o:
#	g++ $(CPPFLAGS) -c ../polyptych/nets.cc -o nets.o
#
#poly.o:
#	g++ $(CPPFLAGS) -c ../polyptych/poly.cc -o poly.o

depends:
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc



.PHONY: clean impositions hidegarbage unhidegarbage
clean:
	rm -f *.o
	
