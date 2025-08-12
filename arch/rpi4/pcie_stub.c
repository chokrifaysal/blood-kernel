/*
 * pcie_stub.c – read/write 32-bit config space
 */

#include "pcie_stub.h"

static volatile u32* ecam(u32 bus, u32 dev, u32 fn, u32 reg) {
    u64 addr = PCIE_ECAM_BASE |
               (bus << 20) |
               (dev << 15) |
               (fn  << 12) |
               (reg & 0xFFF);
    return (volatile u32*)addr;
}

u32 pcie_read(u32 bus, u32 dev, u32 fn, u32 reg) {
    return *ecam(bus, dev, fn, reg);
}

void pcie_write(u32 bus, u32 dev, u32 fn, u32 reg, u32 val) {
    *ecam(bus, dev, fn, reg) = val;
}
