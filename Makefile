# BLOOD_KERNEL v1.1 – safety & OTA
.PHONY: release safety

all: build/kernel.elf

build/kernel.elf: $(wildcard src/kernel/*.c arch/*/*.S)
	@mkdir -p build
	$(CC) $(CFLAGS) -T $(LD_SCRIPT) -o $@ $^

release:
	./tools/build_release.sh

safety:
	./tools/misra_check.sh
	./tests/run_safety.sh
