PRJTYPE = Executable
LIBS = macu glad
ifeq ($(OS), Windows_NT)
	LIBS += glu32 opengl32 msimg32 gdi32 user32
endif
