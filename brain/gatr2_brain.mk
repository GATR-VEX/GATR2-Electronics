# gatr2_brain.mk
# Brain libraries from this repo for a PROS project, built into $(BINDIR)/gatr2/.
#   locagatr  the control board link
# Include it on both sides of common.mk:
#
#   GATR2_ROOT ?= ../..
#   GATR2_BRAIN_LIBS := locagatr
#   include $(GATR2_ROOT)/brain/gatr2_brain.mk
#   -include ./common.mk
#   include $(GATR2_ROOT)/brain/gatr2_brain.mk

ifndef GATR2_BRAIN_SETUP
GATR2_BRAIN_SETUP := 1

# Repository root as a path relative to the PROS project. Drive letter paths
# break the include flags in the toolchain shell.
GATR2_ROOT ?= ../..

# Libraries to build.
GATR2_BRAIN_LIBS ?= locagatr

# Sources per library.
GATR2_SRC_locagatr := \
	$(GATR2_ROOT)/brain/locaGATR/src/link.cpp \
	$(GATR2_ROOT)/brain/locaGATR/pros/smart_port.cpp \
	$(GATR2_ROOT)/pico/locaGATR/src/brainlink/wire.cpp \
	$(GATR2_ROOT)/pico/locaGATR/src/records/encoder.cpp

GATR2_SRC := $(foreach lib,$(GATR2_BRAIN_LIBS),$(GATR2_SRC_$(lib)))
GATR2_OBJ := $(patsubst $(GATR2_ROOT)/%,$(BINDIR)/gatr2/%.o,$(GATR2_SRC))

EXTRA_INCDIR += \
	$(GATR2_ROOT)/brain/locaGATR/include \
	$(GATR2_ROOT)/pico/locaGATR/include
ELF_DEPS += $(GATR2_OBJ)

else

GATR2_WARNFLAGS ?= -Wall -Wextra

# Own compile rule: the common.mk one assumes sources under src/.
define gatr2_src_rule
$(patsubst $(GATR2_ROOT)/%,$(BINDIR)/gatr2/%.o,$1): $1
	$(VV)mkdir -p $$(dir $$@)
	$$(call test_output_2,Compiled $$< ,$(CXX) -c $(INCLUDE) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(GATR2_WARNFLAGS) -MMD -MP -MF $$(@:.o=.d) -o $$@ $$<,$(OK_STRING))
endef
$(foreach src,$(GATR2_SRC),$(eval $(call gatr2_src_rule,$(src))))

-include $(GATR2_OBJ:.o=.d)

endif
