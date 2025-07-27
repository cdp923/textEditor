# Text Editor Makefile
# Supports both Windows (MinGW) and cross-compilation from macOS

# Detect platform and set appropriate compiler
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - use MinGW cross-compiler
    CC = x86_64-w64-mingw32-g++
    WINDRES = x86_64-w64-mingw32-windres
else ifeq ($(OS),Windows_NT)
    # Windows - use native MinGW
    CC = g++
    WINDRES = windres
else
    # Linux or other Unix - use MinGW cross-compiler
    CC = x86_64-w64-mingw32-g++
    WINDRES = x86_64-w64-mingw32-windres
endif

# Project structure
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
RESOURCES_DIR = resources

# Target executable
TARGET = $(BUILD_DIR)/textEditor.exe

# Compiler flags
CPPFLAGS = -I$(INCLUDE_DIR) -DUNICODE -D_UNICODE -DNOMINMAX
CXXFLAGS = -Wall -Wextra -std=c++11 -O2
LDFLAGS = -mwindows -municode
LDLIBS = -lcomdlg32

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Resource files
RC_FILE = $(RESOURCES_DIR)/textEditor.rc
RES_FILE = $(BUILD_DIR)/textEditor.res

# Default target
all: $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile resource file
$(RES_FILE): $(RC_FILE) | $(BUILD_DIR)
	$(WINDRES) $< -O coff -o $@

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Link everything together
$(TARGET): $(OBJECTS) $(RES_FILE)
	$(CC) $(OBJECTS) $(RES_FILE) $(LDFLAGS) $(LDLIBS) -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Rebuild everything
rebuild: clean all

# Install dependencies (macOS only)
install-deps-macos:
	@echo "Installing MinGW cross-compiler for macOS..."
	@if command -v brew >/dev/null 2>&1; then \
		brew install mingw-w64; \
	elif command -v port >/dev/null 2>&1; then \
		sudo port install x86_64-w64-mingw32-gcc; \
	else \
		echo "Please install Homebrew or MacPorts first"; \
		exit 1; \
	fi

# Install Wine (for running on macOS/Linux)
install-wine-macos:
	@echo "Installing Wine for running Windows executables..."
	@if command -v brew >/dev/null 2>&1; then \
		brew install --cask wine-stable; \
	else \
		echo "Please install Homebrew first"; \
		exit 1; \
	fi

# Run the application
run: $(TARGET)
	@if [ "$(UNAME_S)" = "Darwin" ] || [ "$(UNAME_S)" = "Linux" ]; then \
		if command -v wine >/dev/null 2>&1; then \
			wine $(TARGET); \
		else \
			echo "Wine not found. Install with 'make install-wine-macos' or run on Windows"; \
		fi; \
	else \
		$(TARGET); \
	fi

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build (default)
release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

# Show help
help:
	@echo "Text Editor Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all             - Build the application (default)"
	@echo "  clean           - Remove build artifacts"
	@echo "  rebuild         - Clean and build"
	@echo "  debug           - Build with debug symbols"
	@echo "  release         - Build optimized release version"
	@echo "  run             - Build and run the application"
	@echo "  install-deps-macos - Install MinGW cross-compiler on macOS"
	@echo "  install-wine-macos - Install Wine on macOS"
	@echo "  help            - Show this help message"

# Mark phony targets
.PHONY: all clean rebuild debug release run install-deps-macos install-wine-macos help

# Dependencies
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@$(CC) $(CPPFLAGS) -MM $< | sed 's|.*:|$(BUILD_DIR)/$*.o:|' > $@