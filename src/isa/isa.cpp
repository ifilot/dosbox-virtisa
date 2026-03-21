/*
 * DOSBox-VirtIsa custom ISA card emulation hook.
 *
 * This file is intentionally separated from core DOSBox files to make
 * ISA card emulation behavior easy to tune for this fork.
 *
 * This is a hardware level implementation of SlotOtter:
 * https://github.com/ifilot/slot-otter
 */

#include <deque>
#include <fstream>
#include <string>
#include <vector>

#include "dosbox.h"
#include "inout.h"
#include "setup.h"
#include "isa/isa.h"

static const Bitu kIsaPortBase = 0x330;
static const Bitu kIsaPortCount = 4; 		// lower two bits (A0 and A1)
static const bool kIsaDebugTrace = true;

class ISA330Port : public Module_base {
private:
	static ISA330Port *instance; // Singleton pointer used by static I/O callbacks

	std::ifstream sd_image;
	bool has_image;
	bool app_cmd_pending;
	bool sdcs;
	bool sdout;
	Bit8u tx_shift_reg;
	Bit8u rx_shift_reg;
	std::deque<Bit8u> mosi_queue;
	std::deque<Bit8u> miso_queue;

public:
	/**
	 * Constructor
	 *
	 * Initializes the ISA device, optionally opens an SD-card image,
	 * and registers I/O port handlers with DOSBox.
	 *
	 * @param configuration DOSBox configuration section
	 * @param image_path Path to SD card image file (may be null/empty)
	 */
	ISA330Port(Section *configuration, const char *image_path)
	        : Module_base(configuration),
	          has_image(false),
	          app_cmd_pending(false),
	          sdcs(false),
	          sdout(true),
	          tx_shift_reg(0xFF),
	          rx_shift_reg(0xFF) {

		if (image_path && image_path[0]) {
			sd_image.open(image_path, std::ios::in | std::ios::binary);
			has_image = sd_image.good();
			if (has_image) {
				LOG_MSG("[ISA] : SD image loaded from '%s'", image_path);
			} else {
				LOG_MSG("[ISA] : Failed to open SD image '%s' (SD emulation disabled)", image_path);
			}
		} else {
			LOG_MSG("[ISA] : No SD image configured (set isa_sd_image=...)");
		}

		// Register I/O handlers for the configured port range
		for (Bitu port = kIsaPortBase; port < (kIsaPortBase + kIsaPortCount); ++port) {
			IO_RegisterReadHandler(port, ReadPort, IO_MB);
			IO_RegisterWriteHandler(port, WritePort, IO_MB);
		}

		// Store singleton instance for static callbacks
		instance = this;

		LOG_MSG("[ISA] : Registered ISA SD-card handler range 0x%03X-0x%03X",
		        (unsigned int)kIsaPortBase,
		        (unsigned int)(kIsaPortBase + kIsaPortCount - 1));
	}

	/**
	 * Destructor
	 *
	 * Unregisters I/O handlers and clears singleton instance.
	 */
	~ISA330Port() {
		for (Bitu port = kIsaPortBase; port < (kIsaPortBase + kIsaPortCount); ++port) {
			IO_FreeReadHandler(port, IO_MB);
			IO_FreeWriteHandler(port, IO_MB);
		}
		if (instance == this) {
			instance = 0;
		}
	}

	/**
	 * Static I/O read handler
	 *
	 * Called by DOSBox when the emulated CPU reads from one of the ISA ports.
	 * Dispatches to the active instance via the singleton pointer.
	 *
	 * @param port I/O port being read
	 * @param iolen Length of read (unused)
	 * @return Byte returned to emulated CPU
	 */
	static Bitu ReadPort(Bitu port, Bitu /*iolen*/) {
		if (!instance) {
			return 0xFF;
		}

		const Bitu offset = port - kIsaPortBase;
		Bit8u out         = 0xFF;

		// if (kIsaDebugTrace) {
		// 	LOG_MSG("[ISA] IN 0x%03X (0x%02X)", (unsigned int)port, (unsigned int)out);
		// }

		switch (offset) {
			case 0:
				out = instance->rx_shift_reg;
				return out;
			case 1:
				out = instance->rx_shift_reg;
				instance->pulse_8clocks();
				return out;
			case 2:
				instance->set_sdcs(true);
				return 0xFF;	// actually undefined, though most likely pulled to high because of TTL characteristics
			case 3:
				instance->set_sdcs(false);
				return 0xFF;	// actually undefined, though most likely pulled to high because of TTL characteristics
			default: 
				return 0xFF;
		}
	}

	/**
	 * Static I/O write handler
	 *
	 * Called by DOSBox when the emulated CPU writes to one of the ISA ports.
	 * Updates transmit state and triggers SPI clocking.
	 *
	 * @param port I/O port being written
	 * @param val Value written
	 * @param iolen Length of write (unused)
	 */
	static void WritePort(Bitu port, Bitu val, Bitu /*iolen*/) {
		if (!instance)
			return;

		const Bit8u input = (Bit8u)(val & 0xff);
		const Bitu offset  = port - kIsaPortBase;

		switch (offset) {
			case 0:
				// if (kIsaDebugTrace) {
				// 	LOG_MSG("[ISA] OUT 0x%03X (0x%02X)", (unsigned int)port, (unsigned int)input);
				// }
				instance->tx_shift_reg = input;
				instance->pulse_8clocks();
				break;
			case 1:
				// if (kIsaDebugTrace) {
				// 	LOG_MSG("[ISA] OUT 0x%03X (PULSE)", (unsigned int)port);
				// }
				instance->pulse_8clocks();
				break;
			case 2:
				instance->sdout = true;
				break;
			case 3:
				instance->sdout = false;
				break;
			default:
				break;
		}
	}

private:
	/**
	 * Set SD card chip-select state
	 *
	 * Updates SDCS line. !!Currently only logged and not used for logic!!
	 *
	 * @param state New chip-select state
	 */
	void set_sdcs(const bool state) {
		if (sdcs == state)
			return;

		sdcs = state;
	}

	/**
	 * Simulate 8 SPI clock cycles
	 *
	 * Transfers one byte from TX shift register into MOSI queue,
	 * processes commands, and updates RX shift register from MISO queue.
	 */
	void pulse_8clocks() {
		mosi_queue.push_back(tx_shift_reg);
		tx_shift_reg = 0xFF;

		// this function scans for any command words and parses those
		digest_sd();

		if (!miso_queue.empty()) {
			rx_shift_reg = miso_queue.front();
			miso_queue.pop_front();
		} else {
			rx_shift_reg = 0xFF;
		}
	}

	/**
	 * Process accumulated MOSI bytes as SD commands
	 *
	 * Parses 6-byte command frames and generates appropriate responses
	 * into the MISO queue.
	 */
	void digest_sd() {
		// digest sd command, ignores any leading "cleansing bytes" (0xFF)
		if (mosi_queue.size() == 1 && mosi_queue.front() == 0xFF) {
			mosi_queue.pop_front();
			return;
		}

		while (mosi_queue.size() >= 6) {
			Bit8u cmd[6];
			for (Bitu i = 0; i < 6; ++i) {
				cmd[i] = mosi_queue.front();
				mosi_queue.pop_front();
			}

			// CMD00
			if (match_cmd(cmd, 0x40, 0x00, 0x00, 0x00, 0x00, 0x95)) {
				app_cmd_pending = false;
				load_response(make_response_cmd00());
				if (kIsaDebugTrace) {
					LOG_MSG("[ISA] CMD00");
				}
				continue;
			}

			// CMD08
			if (match_cmd(cmd, 0x48, 0x00, 0x00, 0x01, 0xAA, 0x87)) {
				load_response(make_response_cmd08());
				if (kIsaDebugTrace) {
					LOG_MSG("[ISA] CMD08");
				}
				continue;
			}

			// CMD55
			if (match_cmd(cmd, 0x77, 0x00, 0x00, 0x00, 0x00, 0x65)) {
				app_cmd_pending = true;
				load_response(make_response_cmd55());
				if (kIsaDebugTrace) {
					LOG_MSG("[ISA] CMD55");
				}
				continue;
			}

			// ACMD41
			if (app_cmd_pending && match_cmd(cmd, 0x69, 0x40, 0x00, 0x00, 0x00, 0x77)) {
				app_cmd_pending = false;
				load_response(make_response_acmd41());
				if (kIsaDebugTrace) {
					LOG_MSG("[ISA] ACMD41");
				}
				continue;
			}

			// CMD58
			if (match_cmd(cmd, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x01)) {
				load_response(make_response_cmd58());
				if (kIsaDebugTrace) {
					LOG_MSG("[ISA] CMD58");
				}
				continue;
			}

			// CMD17
			if (cmd[0] == (0x40 | 17) && cmd[5] == 0x01) {
				const Bit32u addr = (Bit32u)cmd[1] << 24 | (Bit32u)cmd[2] << 16 |
				                   (Bit32u)cmd[3] << 8 | (Bit32u)cmd[4];
				load_response_cmd17(addr);
				if (kIsaDebugTrace) {
					LOG_MSG("[ISA] CMD17: 0x%08lX", addr);
				}
				continue;
			}

			load_response(make_response_illegal_command());
		}
	}

	/**
	 * Compare a 6-byte command frame against expected values
	 */
	static bool match_cmd(const Bit8u cmd[6],
	                      Bit8u b0, Bit8u b1, Bit8u b2,
	                      Bit8u b3, Bit8u b4, Bit8u b5) {
		return cmd[0] == b0 && cmd[1] == b1 && cmd[2] == b2 &&
		       cmd[3] == b3 && cmd[4] == b4 && cmd[5] == b5;
	}

	/** Generate response for CMD0 (reset) */
	static std::vector<Bit8u> make_response_cmd00() {
		const Bit8u resp[] = {0xFF, 0x01};
		return std::vector<Bit8u>(resp, resp + sizeof(resp));
	}

	/** Generate response for CMD8 (interface condition) */
	static std::vector<Bit8u> make_response_cmd08() {
		const Bit8u resp[] = {0xFF, 0x01, 0x00, 0x00, 0x01, 0xAA};
		return std::vector<Bit8u>(resp, resp + sizeof(resp));
	}

	/** Generate response for CMD55 (app command prefix) */
	static std::vector<Bit8u> make_response_cmd55() {
		const Bit8u resp[] = {0xFF, 0x01};
		return std::vector<Bit8u>(resp, resp + sizeof(resp));
	}

	/** Generate response for ACMD41 (init) */
	static std::vector<Bit8u> make_response_acmd41() {
		const Bit8u resp[] = {0xFF, 0x00};
		return std::vector<Bit8u>(resp, resp + sizeof(resp));
	}

	/** Generate response for CMD58 (read OCR) */
	static std::vector<Bit8u> make_response_cmd58() {
		const Bit8u resp[] = {0xFF, 0x00, 0xC0, 0xFF, 0x80, 0x00};
		return std::vector<Bit8u>(resp, resp + sizeof(resp));
	}

	/** Generate response for illegal/unsupported command */
	static std::vector<Bit8u> make_response_illegal_command() {
		const Bit8u resp[] = {0xFF, 0x04};
		return std::vector<Bit8u>(resp, resp + sizeof(resp));
	}

	/**
	 * Append response bytes to MISO queue
	 */
	void load_response(const std::vector<Bit8u> &resp) {
		for (size_t i = 0; i < resp.size(); ++i) {
			miso_queue.push_back(resp[i]);
		}
	}

	/**
	 * Handle CMD17 (read single block)
	 *
	 * Reads a 512-byte sector from the SD image and pushes it
	 * into the MISO queue along with protocol framing and CRC.
	 *
	 * @param addr Block address (sector index)
	 */
	void load_response_cmd17(Bit32u addr) {
		if (!has_image || !sd_image.is_open()) {
			load_response(make_response_illegal_command());
			return;
		}

		sd_image.clear();
		sd_image.seekg((std::streamoff)(addr * 512UL), std::ios::beg);

		if (!sd_image.good()) {
			load_response(make_response_illegal_command());
			return;
		}

		Bit8u buffer[512];
		sd_image.read((char *)buffer, sizeof(buffer));

		if (sd_image.gcount() != (std::streamsize)sizeof(buffer)) {
			load_response(make_response_illegal_command());
			return;
		}

		// Response + data token + data + CRC
		miso_queue.push_back(0xFF);
		miso_queue.push_back(0x00);
		miso_queue.push_back(0xFF);
		miso_queue.push_back(0xFF);
		miso_queue.push_back(0xFF);
		miso_queue.push_back(0xFF);
		miso_queue.push_back(0xFE);

		for (Bitu i = 0; i < 512; ++i) {
			miso_queue.push_back(buffer[i]);
		}

		const Bit16u checksum = crc16_xmodem(buffer, 512);
		miso_queue.push_back((Bit8u)((checksum >> 8) & 0xFF));
		miso_queue.push_back((Bit8u)(checksum & 0xFF));
	}

	/**
	 * Compute CRC16 (XMODEM variant)
	 *
	 * Used for SD card data block integrity.
	 *
	 * @param data Input buffer
	 * @param len Length in bytes
	 * @return Computed CRC16
	 */
	static Bit16u crc16_xmodem(const Bit8u *data, size_t len) {
		Bit16u crc = 0x0000;
		for (size_t i = 0; i < len; ++i) {
			crc ^= (Bit16u)data[i] << 8;
			for (int j = 0; j < 8; j++) {
				if (crc & 0x8000)
					crc = (Bit16u)((crc << 1) ^ 0x1021);
				else
					crc <<= 1;
			}
		}
		return crc;
	}
};

ISA330Port *ISA330Port::instance = 0;

static ISA330Port *isa_port_handler;
static std::string isa_sd_image_path;

/**
 * Cleanup function for ISA module
 *
 * Deletes the global ISA port handler instance.
 */
static void ISA_Destroy(Section * /*sec*/) {
	delete isa_port_handler;
	isa_port_handler = 0;
}

/**
 * Initialize ISA module
 *
 * Creates (or recreates) the ISA device and registers it with DOSBox.
 * Also preserves SD image configuration across reinitializations.
 *
 * @param sec DOSBox configuration section
 */
void ISA_Init(Section *sec) {
	Section_prop *section = static_cast<Section_prop *>(sec);
	const char *configured_path = section->Get_string("isa_sd_image");

	if (configured_path && configured_path[0])
		isa_sd_image_path = configured_path;

	if (isa_port_handler)
		delete isa_port_handler;

	isa_port_handler = new ISA330Port(sec, isa_sd_image_path.c_str());
	sec->AddDestroyFunction(&ISA_Destroy, true);
}