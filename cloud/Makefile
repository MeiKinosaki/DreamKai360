.SUFFIXES:

ifeq ($(strip $(DEVKITXENON)),)
$(error DEVKITXENON is not set)
endif

include $(DEVKITXENON)/rules

TARGET := dreamkai360_cloud_probe
BUILD := build
SOURCES := .
CFLAGS := -O2 -Wall -Wextra $(MACHDEP) $(INCLUDE)
LDFLAGS := $(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(TARGET).map
LIBS := -lxenon -lm

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(CURDIR)/$(SOURCES)
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(notdir $(wildcard $(SOURCES)/*.c))
OFILES := $(CFILES:.c=.o)
export LD := $(CC)
export INCLUDE := -I$(CURDIR)/$(BUILD) -I$(LIBXENON_INC)
export LIBPATHS := -L$(LIBXENON_LIB)
.PHONY: all clean
all: $(BUILD) $(OUTPUT).elf32
$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).elf32 $(OUTPUT).map
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).elf32: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
