#### MAKEFILE
#### Handcoded by myself.

#### Currently support two platforms:
####  - MacOS
####  - Windows

## Compiler settings.
CXX      := g++
CLANG    := clang++
CXXFLAGS := -std=c++17 -O3 # -Wall -Wextra
## Basic settings.
TARGET   := Tira
BUILDDIR := Build
OUTDIR   := Output
ICDDIR   := Tira
SRCDIR   := Tira
THIRDDIR := Thirdparty
## Add your *.cpp file here, make sure they are under SRCDIR folder.
SOURCES  := $(wildcard $(addprefix $(SRCDIR)/, *.cpp)) $(wildcard $(addprefix $(THIRDDIR)/, *.cpp))
SOURCES  := $(filter-out src/main.cpp, $(SOURCES))
SOURCES  := $(filter-out src/gl.cpp, $(SOURCES))
INCLUDES := $(wildcard $(addprefix $(ICDDIR)/, *.h)) $(wildcard $(addprefix $(THIRDDIR)/, *.h)) $(wildcard $(addprefix $(THIRDDIR)/, *.hpp))
OBJECTS  := $(addprefix $(BUILDDIR)/, $(notdir $(SOURCES:.cpp=.o)))

ifeq ($(OS), Windows_NT)
	MKBDDIR  := if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	MKOUTDIR := if not exist $(OUTDIR) mkdir $(OUTDIR)
	RUN      :=
	PLATFORM := win32
	CLEAN    := if exist $(BUILDDIR) rmdir /s /q $(BUILDDIR)
	CXXFLAGS   := ${CXXFLAGS} -fopenmp
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
		MKBDDIR  := mkdir -p $(BUILDDIR)
		MKOUTDIR := mkdir -p $(OUTDIR)
		RUN      := ./
		PLATFORM := macos
		CLEAN    := rm -rf $(BUILDDIR)
    endif
endif

all: $(PLATFORM)

prepare:
	@$(MKBDDIR)
	@$(MKOUTDIR)

macos: prepare $(OBJECTS) $(INCLUDES)
	@$(CLANG) -o $(TARGET) -framework Cocoa $(CXXFLAGS) Platform/macos.mm $(OBJECTS) -I$(ICDDIR)

win32: prepare $(OBJECTS) $(INCLUDES)
	@$(CXX) -o $(TARGET).exe $(CXXFLAGS) Platform/win32.cpp $(OBJECTS) -I$(ICDDIR) -lgdi32

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(INCLUDES)
	@echo - Build $@
	@$(CXX) $(CXXFLAGS) -I$(ICDDIR) -I$(THIRDDIR) -o $@ -c $<

$(BUILDDIR)/%.o: $(THIRDDIR)/%.cpp $(INCLUDES)
	@echo - Build $@
	@$(CXX) $(CXXFLAGS) -I$(ICDDIR) -I$(THIRDDIR) -o $@ -c $<

clean:
	@$(CLEAN)

run: $(PLATFORM)
	@$(RUN)$(TARGET)
