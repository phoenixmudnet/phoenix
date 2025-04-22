# Generated automatically from Makefile.in by configure.
# CircleMUD 3.0 Makefile.in - Makefile template used by 'configure'
#

# C compiler to use
#CC=purify gcc
#CC=purify cc
CC = gcc
#CC= cc

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -Wshadow -Wnested-externs -ffreestanding -m32
#MYFLAGS = 

#flags for profiling (see hacker.doc for more information)
PROFILE = -L/lib/i386-linux-gnu

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

#CFLAGS = $(MYFLAGS) $(PROFILE)
CFLAGS = -g $(MYFLAGS) $(PROFILE) -DPLAYERS_PORT 

OBJFILES = comm.o \
	act.comm.o \
	act.create.o \
	act.informative.o \
	act.movement.o \
	act.item.o \
	act.offensive.o \
	act.other.o \
	act.other2.o \
	act.social.o \
	act.wizard.o \
	act.wizard2.o \
   assemblies.o \
	alias.o \
	ban.o \
	boards.o \
	buffer.o \
	castle.o \
	clan.o \
	class.o \
	color.o \
	config.o \
	constants.o \
	corpse.o \
	db.o \
	fight.o \
	graph.o \
	gremort_exam.o \
	guild.o \
	handler.o \
	house.o \
	ident.o \
	interpreter.o \
	limits.o \
	magic.o \
	mail.o \
	mobact.o \
	mobcmd.o \
	mobprog.o \
	modify.o \
	objact.o \
	objsave.o \
	olc.o \
	osearch.o \
	path.o \
	queue.o \
	race.o \
	random.o \
	shop.o \
	skills.o \
	spec_assign.o \
	spec_procs.o \
	spell_parser.o \
	spells.o \
	spelledit.o \
	utils.o \
	weather.o \
	assedit.o \
	gedit.o \
	hedit.o \
	medit.o \
	oedit.o \
	redit.o \
	sedit.o \
	zedit.o \
	dg_comm.o \
	dg_db_scripts.o \
	dg_event.o \
	dg_handler.o \
	dg_mobcmd.o \
	dg_objcmd.o \
	dg_scripts.o \
	dg_triggers.o \
	dg_wldcmd.o \
	dg_olc.o \
	dg_misc.o

default: .accepted
	$(MAKE) ../compile/circle

.accepted:
	@./licheck more

utils: .accepted
	$(MAKE) ../bin/autowiz
	$(MAKE) ../bin/delobjs
	$(MAKE) ../bin/listrent
	$(MAKE) ../bin/mudpasswd
	$(MAKE) ../bin/play2to3
	$(MAKE) ../bin/purgeplay
#	$(MAKE) ../bin/shopconv
	$(MAKE) ../bin/showplay
	$(MAKE) ../bin/findsite
	$(MAKE) ../bin/sign
	$(MAKE) ../bin/split
	$(MAKE) ../bin/wld2html

all: .accepted
	$(MAKE) ../compile/circle
	$(MAKE) utils

circle:
	$(MAKE) ../compile/circle
autowiz:
	$(MAKE) ../bin/autowiz
delobjs:
	$(MAKE) ../bin/delobjs
listrent:
	$(MAKE) ../bin/listrent
mudpasswd:
	$(MAKE) ../bin/mudpasswd
play2to3:
	$(MAKE) ../bin/play2to3
purgeplay:
	$(MAKE) ../bin/purgeplay
shopconv:
	$(MAKE) ../bin/shopconv
showplay:
	$(MAKE) ../bin/showplay
findsite:
	$(MAKE) ../bin/findsite
sign:
	$(MAKE) ../bin/sign
split:
	$(MAKE) ../bin/split
wld2html:
	$(MAKE) ../bin/wld2html


../bin/autowiz: util/autowiz.c conf.h sysdep.h structs.h utils.h buffer.h db.h
	$(CC) $(CFLAGS) -o ../../circle/bin/autowiz util/autowiz.c
../bin/delobjs: util/delobjs.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o ../../circle/bin/delobjs util/delobjs.c
../bin/listrent: util/listrent.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o ../../circle/bin/listrent util/listrent.c
../bin/mudpasswd: util/mudpasswd.c conf.h sysdep.h structs.h utils.h buffer.h
	$(CC) $(CFLAGS) -o ../../circle/bin/mudpasswd util/mudpasswd.c 
../bin/play2to3: util/play2to3.c
	$(CC) $(CFLAGS) -o ../../circle/bin/play2to3 util/play2to3.c
../bin/purgeplay: util/purgeplay.c conf.h sysdep.h structs.h utils.h buffer.h
	$(CC) $(CFLAGS) -o ../../circle/bin/purgeplay util/purgeplay.c
../bin/shopconv: util/shopconv.c conf.h sysdep.h structs.h db.h utils.h buffer.h shop.h
	$(CC) $(CFLAGS) -o ../../circle/bin/shopconv util/shopconv.c
../bin/showplay: util/showplay.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o ../../circle/bin/showplay util/showplay.c
../bin/findsite: util/findsite.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o ../../circle/bin/findsite util/findsite.c
../bin/sign: util/sign.c conf.h sysdep.h
	$(CC) $(CFLAGS) -o ../../circle/bin/sign util/sign.c 
../bin/split: util/split.c
	$(CC) $(CFLAGS) -o ../../circle/bin/split util/split.c
../bin/wld2html: util/wld2html.c
	$(CC) $(CFLAGS) -o ../../circle/bin/wld2html util/wld2html.c

../compile/circle : $(OBJFILES)
	mkdir -p ../../circle/compile ../../circle/bin
	$(CC) -m32 -g -o ../../circle/compile/circle $(PROFILE) $(OBJFILES)
	cp ../../circle/compile/circle ../../circle/bin
clean:
	rm -f *.o *~

# Dependencies for the object files (automagically generated with
# gcc -MM)

act.comm.o: act.comm.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h screen.h dg_scripts.h constants.h
	$(CC) -c $(CFLAGS) act.comm.c
act.create.o: act.create.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h screen.h
	$(CC) -c $(CFLAGS) act.create.c
act.informative.o: act.informative.c conf.h sysdep.h structs.h utils.h \
	buffer.h comm.h interpreter.h handler.h db.h spells.h screen.h clan.h \
	constants.h dg_scripts.h
	$(CC) -c $(CFLAGS) act.informative.c
act.item.o: act.item.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h clan.h dg_scripts.h constants.h
	$(CC) -c $(CFLAGS) act.item.c
act.movement.o: act.movement.c conf.h sysdep.h structs.h utils.h buffer.h \
	comm.h interpreter.h handler.h db.h spells.h house.h dg_scripts.h \
	constants.h clan.h
	$(CC) -c $(CFLAGS) act.movement.c
act.offensive.o: act.offensive.c conf.h sysdep.h structs.h utils.h buffer.h \
	comm.h interpreter.h handler.h db.h spells.h constants.h
	$(CC) -c $(CFLAGS) act.offensive.c
act.other.o: act.other.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h screen.h house.h ident.h \
	clan.h dg_scripts.h constants.h
	$(CC) -c $(CFLAGS) act.other.c
act.other2.o: act.other2.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h screen.h house.h ident.h \
	clan.h dg_scripts.h constants.h
	$(CC) -c $(CFLAGS) act.other2.c
act.social.o: act.social.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h constants.h
	$(CC) -c $(CFLAGS) act.social.c
act.wizard.o: act.wizard.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h house.h screen.h spec_assign.h \
	clan.h dg_scripts.h constants.h
	$(CC) -c $(CFLAGS) act.wizard.c
act.wizard2.o: act.wizard2.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h house.h screen.h spec_assign.h \
	clan.h dg_scripts.h constants.h
	$(CC) -c $(CFLAGS) act.wizard2.c
assemblies.o: assemblies.c structs.h utils.h buffer.h interpreter.h
	$(CC) -c $(CFLAGS) assemblies.c
alias.o: alias.c structs.h utils.h buffer.h interpreter.h
	$(CC) -c $(CFLAGS) alias.c
ban.o: ban.c conf.h sysdep.h structs.h utils.h buffer.h comm.h interpreter.h \
	handler.h db.h
	$(CC) -c $(CFLAGS) ban.c
boards.o: boards.c conf.h sysdep.h structs.h utils.h buffer.h comm.h db.h \
	boards.h interpreter.h handler.h clan.h
	$(CC) -c $(CFLAGS) boards.c
buffer.o: buffer.c conf.h sysdep.h structs.h utils.h buffer.h
	$(CC) -c $(CFLAGS) buffer.c
castle.o: castle.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h
	$(CC) -c $(CFLAGS) castle.c
class.o: class.c conf.h sysdep.h structs.h db.h utils.h buffer.h spells.h \
	interpreter.h constants.h
	$(CC) -c $(CFLAGS) class.c
clan.o: clan.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h screen.h clan.h constants.h
	$(CC) -c $(CFLAGS) clan.c
color.o: color.c conf.h sysdep.h structs.h utils.h buffer.h comm.h
	$(CC) -c $(CFLAGS) color.c
comm.o: comm.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h house.h ident.h queue.h dg_scripts.h \
	constants.h
	$(CC) -c $(CFLAGS) comm.c
config.o: config.c conf.h sysdep.h structs.h
	$(CC) -c $(CFLAGS) config.c
constants.o: constants.c conf.h sysdep.h structs.h
	$(CC) -c $(CFLAGS) constants.c
corpse.o: corpse.c conf.h sysdep.h structs.h comm.h handler.h db.h \
	interpreter.h utils.h spells.h constants.h buffer.h
	$(CC) -c $(CFLAGS) corpse.c
db.o: db.c conf.h sysdep.h structs.h utils.h buffer.h db.h comm.h handler.h \
	spells.h mail.h interpreter.h house.h spec_assign.h dg_scripts.h \
	constants.h
	$(CC) -c $(CFLAGS) db.c
fight.o: fight.c conf.h sysdep.h structs.h utils.h buffer.h comm.h handler.h \
	interpreter.h db.h spells.h screen.h dg_scripts.h constants.h
	$(CC) -c $(CFLAGS) fight.c
graph.o: graph.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h
	$(CC) -c $(CFLAGS) graph.c
guild.o: guild.c conf.h sysdep.h structs.h comm.h handler.h db.h \
	interpreter.h utils.h buffer.h spells.h guild.h constants.h
	$(CC) -c $(CFLAGS) guild.c
gremort_exam.o: gremort_exam.c act.comm.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h screen.h dg_scripts.h constants.h
	$(CC) -c $(CFLAGS) gremort_exam.c
handler.o: handler.c conf.h sysdep.h structs.h utils.h buffer.h comm.h db.h \
	handler.h interpreter.h spells.h dg_scripts.h queue.h constants.h
	$(CC) -c $(CFLAGS) handler.c
house.o: house.c conf.h sysdep.h structs.h comm.h handler.h db.h \
	interpreter.h utils.h buffer.h house.h constants.h
	$(CC) -c $(CFLAGS) house.c
ident.o: ident.c conf.h sysdep.h structs.h utils.h buffer.h comm.h db.h \
	ident.h
	$(CC) -c $(CFLAGS) ident.c
interpreter.o: interpreter.c conf.h sysdep.h structs.h comm.h interpreter.h \
	db.h utils.h buffer.h spells.h handler.h mail.h screen.h queue.h \
	dg_scripts.h
	$(CC) -c $(CFLAGS) interpreter.c
limits.o: limits.c conf.h sysdep.h structs.h utils.h buffer.h spells.h comm.h \
	db.h handler.h constants.h
	$(CC) -c $(CFLAGS) limits.c
magic.o: magic.c vnum.h conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	spells.h handler.h db.h
	$(CC) -c $(CFLAGS) magic.c
mail.o: mail.c conf.h sysdep.h structs.h utils.h buffer.h comm.h db.h \
	interpreter.h handler.h mail.h
	$(CC) -c $(CFLAGS) mail.c
mobact.o: mobact.c conf.h sysdep.h structs.h utils.h buffer.h db.h comm.h \
	interpreter.h handler.h spells.h queue.h
	$(CC) -c $(CFLAGS) mobact.c
mobcmd.o: mobcmd.c conf.h sysdep.h structs.h utils.h buffer.h db.h comm.h \
	interpreter.h handler.h spells.h
	$(CC) -c $(CFLAGS) mobcmd.c
mobprog.o: mobprog.c conf.h sysdep.h structs.h utils.h buffer.h db.h comm.h \
	interpreter.h handler.h spells.h
	$(CC) -c $(CFLAGS) mobprog.c
modify.o: modify.c conf.h sysdep.h structs.h utils.h buffer.h interpreter.h \
	handler.h db.h comm.h spells.h mail.h boards.h
	$(CC) -c $(CFLAGS) modify.c
objact.o: objact.c conf.h sysdep.h structs.h utils.h buffer.h db.h comm.h \
	interpreter.h handler.h spells.h queue.h screen.h
	$(CC) -c $(CFLAGS) objact.c
objsave.o: objsave.c conf.h sysdep.h structs.h comm.h handler.h db.h \
	interpreter.h utils.h buffer.h spells.h
	$(CC) -c $(CFLAGS) objsave.c
olc.o: olc.c conf.h sysdep.h structs.h utils.h buffer.h comm.h interpreter.h \
	handler.h db.h olc.h screen.h dg_olc.h
	$(CC) -c $(CFLAGS) olc.c
osearch.o: osearch.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h
	$(CC) -c $(CFLAGS) osearch.c
path.o: path.c conf.h sysdep.h structs.h comm.h handler.h db.h \
	interpreter.h utils.h buffer.h path.h
	$(CC) -c $(CFLAGS) path.c
queue.o: queue.c structs.h utils.h buffer.h db.h comm.h interpreter.h \
	spells.h handler.h queue.h
	$(CC) -c $(CFLAGS) queue.c
race.o: race.c sysdep.h conf.h structs.h db.h interpreter.h utils.h buffer.h \
	comm.h
	$(CC) -c $(CFLAGS) race.c                                  
random.o: random.c
	$(CC) -c $(CFLAGS) random.c
shop.o: shop.c conf.h sysdep.h structs.h comm.h handler.h db.h interpreter.h \
	utils.h buffer.h shop.h constants.h dg_scripts.h
	$(CC) -c $(CFLAGS) shop.c
skills.o: skills.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h constants.h
	$(CC) -c $(CFLAGS) skills.c
spec_assign.o: spec_assign.c conf.h sysdep.h structs.h db.h interpreter.h \
	utils.h buffer.h spec_assign.h
	$(CC) -c $(CFLAGS) spec_assign.c
spec_procs.o: spec_procs.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	interpreter.h handler.h db.h spells.h constants.h dg_scripts.h shop.h\
	clan.h
	$(CC) -c $(CFLAGS) spec_procs.c
spell_parser.o: spell_parser.c vnum.h conf.h sysdep.h structs.h utils.h \
	buffer.h interpreter.h spells.h handler.h comm.h db.h
	$(CC) -c $(CFLAGS) spell_parser.c
spells.o: spells.c vnum.h conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	spells.h handler.h db.h clan.h constants.h
	$(CC) -c $(CFLAGS) spells.c
spelledit.o: spelledit.c structs.h buffer.h comm.h handler.h db.h interpreter.h utils.h spells.h constants.h
	$(CC) -c $(CFLAGS) spelledit.c
utils.o: utils.c conf.h sysdep.h structs.h utils.h buffer.h comm.h screen.h \
	spells.h handler.h
	$(CC) -c $(CFLAGS) utils.c
weather.o: weather.c conf.h sysdep.h structs.h utils.h buffer.h comm.h \
	handler.h interpreter.h db.h
	$(CC) -c $(CFLAGS) weather.c

assedit.o: assedit.c conf.h sysdep.h structs.h comm.h utils.h buffer.h spells.h \
	db.h guild.h olc.h
	$(CC) -c $(CFLAGS) assedit.c 
gedit.o: gedit.c conf.h sysdep.h structs.h comm.h utils.h buffer.h spells.h \
	db.h guild.h olc.h
	$(CC) -c $(CFLAGS) gedit.c 
hedit.o: hedit.c structs.h comm.h utils.h buffer.h spells.h \
	db.h guild.h olc.h
	$(CC) -c $(CFLAGS) hedit.c 
medit.o: medit.c conf.h sysdep.h structs.h utils.h buffer.h comm.h spells.h \
	shop.h db.h olc.h handler.h spec_assign.h dg_olc.h constants.h
	$(CC) -c $(CFLAGS) medit.c 
oedit.o: oedit.c conf.h sysdep.h structs.h utils.h buffer.h comm.h boards.h \
	spells.h db.h olc.h shop.h dg_olc.h constants.h
	$(CC) -c $(CFLAGS) oedit.c
redit.o: redit.c conf.h sysdep.h structs.h utils.h buffer.h comm.h boards.h \
	olc.h dg_olc.h constants.h
	$(CC) -c $(CFLAGS) redit.c
sedit.o: sedit.c conf.h sysdep.h structs.h utils.h buffer.h comm.h db.h \
	olc.h shop.h
	$(CC) -c $(CFLAGS) sedit.c 
zedit.o: zedit.c conf.h sysdep.h structs.h utils.h buffer.h comm.h db.h olc.h \
	constants.h
	$(CC) -c $(CFLAGS) zedit.c 

dg_comm.o: dg_comm.c conf.h sysdep.h structs.h dg_scripts.h utils.h comm.h \
	handler.h db.h buffer.h
	$(CC) -c $(CFLAGS) dg_comm.c
dg_db_scripts.o: dg_db_scripts.c conf.h sysdep.h structs.h dg_scripts.h \
	utils.h comm.h handler.h db.h dg_event.h buffer.h
	$(CC) -c $(CFLAGS) dg_db_scripts.c
dg_event.o: dg_event.c conf.h sysdep.h dg_event.h utils.h structs.h buffer.h \
	queue.h
	$(CC) -c $(CFLAGS) dg_event.c
dg_handler.o: dg_handler.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
	comm.h handler.h db.h dg_event.h buffer.h spells.h
	$(CC) -c $(CFLAGS) dg_handler.c
dg_misc.o: dg_misc.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
	comm.h handler.h db.h interpreter.h comm.h spells.h buffer.h \
	dg_event.h screen.h 
	$(CC) -c $(CFLAGS) dg_misc.c
dg_mobcmd.o: dg_mobcmd.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
	comm.h handler.h db.h interpreter.h comm.h spells.h buffer.h
	$(CC) -c $(CFLAGS) dg_mobcmd.c
dg_objcmd.o: dg_objcmd.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
	comm.h handler.h db.h interpreter.h comm.h buffer.h
	$(CC) -c $(CFLAGS) dg_objcmd.c
dg_scripts.o: dg_scripts.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
	comm.h handler.h db.h comm.h screen.h buffer.h constants.h
	$(CC) -c $(CFLAGS) dg_scripts.c
dg_triggers.o: dg_triggers.c conf.h sysdep.h structs.h dg_scripts.h \
	utils.h comm.h handler.h db.h interpreter.h buffer.h
	$(CC) -c $(CFLAGS) dg_triggers.c
dg_wldcmd.o: dg_wldcmd.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
	comm.h handler.h db.h comm.h interpreter.h buffer.h
	$(CC) -c $(CFLAGS) dg_wldcmd.c
dg_olc.o: dg_olc.c conf.h sysdep.h structs.h dg_scripts.h utils.h \
	comm.h db.h olc.h dg_olc.h dg_event.h buffer.h
	$(CC) -c $(CFLAGS) dg_olc.c

