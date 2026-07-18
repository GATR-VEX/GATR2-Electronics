# Brain

Libraries for the V5 Brain. Nothing here moves the robot.

- [locaGATR](locaGATR/README.md): the Brain end of the control board link. It
  asks the board for its readings over RS-485 and decodes them. The board end
  is [pico/locaGATR](../pico/locaGATR/README.md); the pose is computed by the
  Localizer in GATR2-Override (`lib/locaGATR`).
- [gatr2_brain.mk](gatr2_brain.mk): compiles the library into a PROS project
  straight from this repository.

## Add it to a PROS project

With this repository cloned next to the robot code, in the PROS project's
`Makefile`:

```make
# this repository, as a relative path with no drive letter
GATR2_ROOT ?= ../../GATR2-Electronics
GATR2_BRAIN_LIBS := locagatr

# at the bottom, gatr2_brain.mk on both sides of common.mk
include $(GATR2_ROOT)/brain/gatr2_brain.mk
-include ./common.mk
include $(GATR2_ROOT)/brain/gatr2_brain.mk
```

## Host build

```sh
cmake -S brain -B build-brain
cmake --build build-brain --parallel
ctest --test-dir build-brain --output-on-failure
```

Builds and runs the locaGATR tests.
