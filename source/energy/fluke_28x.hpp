#pragma once
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

// Linux headers
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "energy/um25c.hpp"
#include "util/utilities.hpp"

namespace Energy {

class fluke_28x : public MeasureDevice {
private:
	int m_pointer;
	struct termios m_old_termios;
	std::string m_unit = "ADC";  // "VDC"
	double m_voltage = 0.0;

public:
	fluke_28x(std::string port = "/dev/ttyUSB0", double voltage = 5.0)
	    : m_voltage(voltage)
	{
		m_pointer = open(port.c_str(), O_RDWR | O_NOCTTY);
		if (m_pointer < 0) {
			throw std::runtime_error("Could not open serial device");
		}

		tcgetattr(m_pointer, &m_old_termios);
		struct termios new_tio = m_old_termios;
		new_tio.c_iflag = 0;
		new_tio.c_oflag = 0;
		new_tio.c_lflag = 0;
		new_tio.c_cc[VMIN] = 1;
		new_tio.c_cc[VTIME] = 0;
		cfsetspeed(&new_tio, B115200);
		tcflush(m_pointer, TCIFLUSH | TCIOFLUSH);
		tcsetattr(m_pointer, TCSANOW, &new_tio);

		// timeout ?
	}

	std::string read_step()
	{
		char val[32], single;
		memset(&val, '\0', sizeof(val));
		for (size_t i = 0; i < 32; i++) {
			int n = ::read(m_pointer, &single, sizeof(single));
			if (single == '\r') {
				return std::string(val);
			}

			if (n == 0) {
				throw std::runtime_error("Could not read serial device");
			}
			val[i] = single;
		}
		return std::string(val);
	}

	double read()
	{
		auto resp = read_step();
		if (resp != "0") {
			if (resp == "E") {
				return nan("");
			}
			throw std::runtime_error("Could not read serial device");
		}
		resp = read_step();
		auto val_vec = SNAB::Utilities::split(resp, ',');
		if (val_vec[2] != "NORMAL") {
			return nan("");
		}

		return std::stof(val_vec[0]);
	}

	void write()
	{
		unsigned char msg[] = {'Q', 'M', '\r'};
		int ret = ::write(m_pointer, msg, sizeof(msg));
		if (ret != sizeof(msg)) {
			throw std::runtime_error("Could not write to serial device");
		}
	}

	data get_data_sample_timed()
	{
		write();
		while (true) {
			auto val = read();
			if (!isnan(val)) {
				return {std::chrono::steady_clock::now(), m_voltage * 1e3,
				        double(val * 1e3), double(m_voltage * 1e3 * val)};
			}
		}
	}

	~fluke_28x()
	{
		tcflush(m_pointer, TCIFLUSH);
		tcsetattr(m_pointer, TCSANOW, &m_old_termios);
		close(m_pointer);
	}
};
}  // namespace Energy
