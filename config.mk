PRJTYPE = Executable
LIBS = macu glad
ifeq ($(OS), Windows_NT)
	LIBS += glu32 opengl32 msimg32 gdi32 user32
else
	LIBS += GL X11 Xrender pthread dl
endif
EXTDEPS = glad::0.1.12a0 macu::0.0.2dev
