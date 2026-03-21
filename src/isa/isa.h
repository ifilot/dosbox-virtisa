/*
 * DOSBox-VirtIsa custom ISA card emulation hook.
 *
 * This file is intentionally separated from core DOSBox files to make
 * ISA card emulation behavior easy to tune for this fork.
 *
 * This is a hardware level implementation of SlotOtter:
 * https://github.com/ifilot/slot-otter
 */

#ifndef DOSBOX_VIRTISA_ISA_H
#define DOSBOX_VIRTISA_ISA_H

class Section;

void ISA_Init(Section *sec);

#endif
