PLATFORM ?= PLATFORM_DESKTOP

RAYLIB_SRC_PATH ?= ./src/external/raylib/src/
RAYGUI_H_PATH   ?= ./src/external/raygui/
RAYLIB_LIB      ?= ./libraylib.a

RAW_BUILD_PATH  ?= ./build/
BUILD_PATH      ?= $(RAW_BUILD_PATH)
ASSETS_PATH     ?= ./assets/
OUTPUT_FILE     ?= cand

RAYLIB_PATH     ?= $(BUILD_PATH)$(RAYLIB_LIB)
SHELL_PATH      ?= $(ASSETS_PATH)minshell.html

SRC_DIR         ?= ./src/

INCLUDE_PATHS   ?= -I$(RAYLIB_SRC_PATH) -I$(RAYGUI_H_PATH)

CC ?= cc

ADDITIONAL_FLAGS ?= -Wall -Wextra

UNAMEOS = $(shell uname)

ifeq ($(PLATFORM),PLATFORM_DESKTOP)
	ifeq ($(OS),Windows_NT)
		BUILD_PATH = $(RAW_BUILD_PATH)Windows/
	else
    	ifeq ($(UNAMEOS),Darwin)
    	    BUILD_PATH = $(RAW_BUILD_PATH)Darwin/
    		ADDITIONAL_FLAGS += -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
    	endif
    	ifeq ($(UNAMEOS),Linux)
    		BUILD_PATH = $(RAW_BUILD_PATH)Linux/
    	endif
     endif
endif
ifeq ($(PLATFORM),PLATFORM_WEB)
	BUILD_PATH = $(RAW_BUILD_PATH)Wasm/
	RAYLIB_LIB = /libraylib.web.a
	OUTPUT_FILE = cand.html
	ADDITIONAL_FLAGS += -Os -s USE_GLFW=3 --shell-file $(SHELL_PATH) --preload-file $(ASSETS_PATH)
	CC = emcc
endif

cand: $(SRC_DIR)main.c $(RAYLIB_LIB) build
	$(CC) $(SRC_DIR)main.c $(RAYLIB_PATH) $(INCLUDE_PATHS) $(ADDITIONAL_FLAGS) -o $(BUILD_PATH)$(OUTPUT_FILE)

$(RAYLIB_LIB): build
	make -C $(RAYLIB_SRC_PATH) PLATFORM=$(PLATFORM)
	cp $(RAYLIB_SRC_PATH)$(RAYLIB_LIB) $(BUILD_PATH)

build:
	mkdir -p $(BUILD_PATH)

clean:
	rm -rf $(RAW_BUILD_PATH)
	make clean -C $(RAYLIB_SRC_PATH)
