################################################################################
#|  Project: Talaria
#|     Date: 2020-06-23
#|   Author: Dan Bechard
################################################################################

# Input
INCLUDES := -Isrc -Iinclude
SRC_DIR := src
SRC_FILES := main.c
SOURCES := $(SRC_FILES:%=$(SRC_DIR)/%)
#LIBS = -Llib -lmingw32 -lSDL2main -lSDL2 -lopengl32 -mwindows # Use this with MinGW libs
LIBS := -Llib/Win64 -lShell32 -lSDL2main -lSDL2 -Xlinker /subsystem:windows -lopengl32 -lopenal32 -lole32 -lfmodL_vc # Use this with MSVC libs

# Output
OBJ_DIR := clang/obj
OBJECTS := $(subst $(SRC_DIR)/,$(OBJ_DIR)/,$(SOURCES:.c=.o))
BIN_DIR := clang/bin
BIN_EXE := $(BIN_DIR)/talaria64_d.exe

# Compiler & flags
# Optimization flags:
# -O0 Disabled
# -Og Debug
# -O2 Release
# -O3 Extreme (Careful, might make EXE bigger or invoke undefined behavior!)
SHARED_FLAGS = -std=c99 -g -O0 -Wall -Wextra -Werror -Wno-unused-function -Wno-missing-braces #-Wno-missing-field-initializers -Wno-deprecated-declarations #-Wno-error=incompatible-pointer-types
GCC_FLAGS = -fmax-errors=3
GCC_FLAGS_LINUX = -fsanitize=address -fno-omit-frame-pointer
CLANG_FLAGS = -ferror-limit=3 -fcolor-diagnostics -Wno-macro-redefined
#CC = gcc #-v
CC = clang
#CFLAGS = $(GCC_FLAGS) $(SHARED_FLAGS)
CFLAGS = $(CLANG_FLAGS) $(SHARED_FLAGS)
LDFLAGS = # None

default: banner make-build-dirs $(BIN_EXE)

banner:
	$(info ========================================)
	$(info #  _______    _            _           #)
	$(info # |__   __|  | |          |_|          #)
	$(info #    | | __ _| | __ _ _ __ _  __ _     #)
	$(info #    | |/ _` | |/ _` | '__| |/ _` |    #)
	$(info #    | | |_| | | |_| | |  | | |_| |    #)
	$(info #    |_|\__,_|_|\__,_|_|  |_|\__,_|    #)
	$(info #                                      #)
	$(info #     Copyright 2020 - Dan bechard     #)
	$(info ========================================)

################################################################################
# Link executable
$(BIN_EXE): $(OBJECTS)
	$(info [EXE] $@)
	$(foreach O,$^,$(info +  [OBJ] ${O}))
	@$(CC) -o $@ $^ $(LIBS)

# Compile C files into OBJ files and generate dependencies
$(OBJECTS): $(SOURCES)
	$(info [OBJ] $@)
	$(foreach S,$^,$(info +  [SRC] ${S}))
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

make-build-dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Delete all generated files
.PHONY: clean
clean:
	$(info )
	$(info ---- Clean [clean] -------------------------------------------------)
	$(info [DEL] $(BIN_EXE))
	-@rm $(BIN_EXE)
	$(info [DEL] $(OBJ_DIR)/*)
	-@rm -r $(OBJ_DIR)/*

################################################################################
# Miscellaneous notes
################################################################################
#|  $@ File name of target
#|  $< Name of first prerequisite
#|  $^ Name of all prerequisites, with spaces between them
#|  $? Name of all prerequisites which have changed
#|  %  Wildcard, can be neighbored by prefix, suffix, or both
################################################################################

# Note: To use this, run e.g. `make print-SOURCES`
print-%  : ; @echo $* = $($*)
