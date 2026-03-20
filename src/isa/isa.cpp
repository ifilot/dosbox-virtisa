/*
 * DOSBox-VirtIsa custom ISA card emulation hook.
 *
 * This file is intentionally separated from core DOSBox files to make
 * ISA card emulation behavior easy to tune for this fork.
 */

#include "dosbox.h"
#include "inout.h"
#include "setup.h"
#include "isa/isa.h"

static const Bitu kIsaPort = 0x330;
static const Bitu kIsaShift = 1;

class ISA330Port : public Module_base {
public:
	ISA330Port(Section *configuration) : Module_base(configuration) {
		/*
		 * VirtIsa custom insertion:
		 * Register read/write handlers for I/O port 0x330 so this fork can
		 * emulate a minimal ISA card command/data interaction point.
		 */
		IO_RegisterReadHandler(kIsaPort, ReadPort, IO_MB);
		IO_RegisterWriteHandler(kIsaPort, WritePort, IO_MB);
		LOG_MSG("[DEBUG]: Registered custom ISA port handler at 0x%03X", (unsigned int)kIsaPort);
	}

	~ISA330Port() {
		IO_FreeReadHandler(kIsaPort, IO_MB);
		IO_FreeWriteHandler(kIsaPort, IO_MB);
	}

	static Bitu ReadPort(Bitu /*port*/, Bitu /*iolen*/) {
		LOG_MSG("[DEBUG]: read from 0x%03X -> 0x%02X",
		        (unsigned int)kIsaPort, (unsigned int)shifted_value);
		return shifted_value;
	}

	static void WritePort(Bitu /*port*/, Bitu val, Bitu /*iolen*/) {
		const Bit8u input = (Bit8u)(val & 0xff);
		shifted_value = (Bit8u)(input << kIsaShift);
		LOG_MSG("[DEBUG]: write to 0x%03X value 0x%02X -> shifted/stored 0x%02X",
		        (unsigned int)kIsaPort,
		        (unsigned int)input, (unsigned int)shifted_value);
	}

private:
	static Bit8u shifted_value;
};

Bit8u ISA330Port::shifted_value = 0;

static ISA330Port *isa_port_handler;

static void ISA_Destroy(Section * /*sec*/) {
	delete isa_port_handler;
	isa_port_handler = 0;
}

void ISA_Init(Section *sec) {
	/*
	 * VirtIsa custom insertion:
	 * Initialize ISA 0x330 emulation module during DOSBox startup.
	 */
	LOG_MSG("[DEBUG]: initializing ISA 0x330 handler (this can override MPU-401 at the same port).");
	isa_port_handler = new ISA330Port(sec);
	sec->AddDestroyFunction(&ISA_Destroy, true);
}
