# Brain

V5 Brain libraries.

- [locaGATR](locaGATR/README.md): the Brain end of the control board link.
- [gatr2_brain.mk](gatr2_brain.mk): builds it into a PROS project from this
  repo.

## PROS project

With this repo next to the robot code, in the project `Makefile`:

```make
# relative path, no drive letter
GATR2_ROOT ?= ../../GATR2-Electronics
GATR2_BRAIN_LIBS := locagatr

# at the bottom, on both sides of common.mk
include $(GATR2_ROOT)/brain/gatr2_brain.mk
-include ./common.mk
include $(GATR2_ROOT)/brain/gatr2_brain.mk
```

## Host tests

```sh
cmake -S brain -B build-brain
cmake --build build-brain --parallel
ctest --test-dir build-brain --output-on-failure
```
