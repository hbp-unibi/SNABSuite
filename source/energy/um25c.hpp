/*******
 * um25c
 *
 * Larger parts of this file are taken from
 * https://github.com/davatorium/um25c-client
 * Thus, the following license applies to this file:
 *
 * MIT/X11 License
 * Copyright © 2019 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once
#include <fcntl.h>
#include <netinet/in.h>  // arpa?
#include <termios.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace Energy {

class MeasureDevice {
public:
	// Time, millivolts, milliamps, milliwatts
	typedef std::tuple<std::chrono::time_point<std::chrono::steady_clock>,
	                   double, double, double>
	    data;
	virtual data get_data_sample_timed() = 0;

	virtual ~MeasureDevice() {}
};

/**
 * @brief Class for reading data from UM25C, a usbmeter measuring voltage and
 * current used by an usb device. To make use of this class, the user has to
 * make sure to prepare the system:
 *
 * 1. Get the address of the the bluetooth device, for example by using `hcitool
 * scan`; look out for `UM25C`
 * 2. Call `sudo rfcomm bind 0 <address>`, where <address> has to be replaced
 * with the one gotten in 1.
 * 3. Change permission of the device by calling `sudo chown <user>
 * /dev/rfcomm0`, where user has to be replaced by your username.
 *
 * Note: 2. can be called without sudo and 3. ignored if you have appropriate
 * rights to access. If not, contact your admin.
 *
 */
class um25c : public MeasureDevice {
private:
	int m_pointer;
	struct termios m_old_termios;

	const uint8_t msg_data_dump = 0xf0;
	const uint8_t msg_clear_sum = 0xf4;
	const uint8_t msg_data_group = 0xa0;
	const uint8_t data_dump_length = 130;

public:
	/**
	 * @brief Constructor, setting up the bluetooth device
	 *
	 * @param port string to port, where the um25c is binded to
	 */
	um25c(std::string port = "/dev/rfcomm0")
	{
		m_pointer = open(port.c_str(), O_RDWR | O_NOCTTY);
		if (m_pointer < 0) {
			throw std::runtime_error("Could not open Bluetooth device");
		}
		tcgetattr(m_pointer, &m_old_termios);
		struct termios new_tio;
		new_tio.c_cflag = B9600 | CS8 | CREAD | PARODD;
		new_tio.c_iflag = 0;
		new_tio.c_oflag = 0;
		new_tio.c_lflag = 0;  // ICANON;
		new_tio.c_cc[VMIN] = 1;
		new_tio.c_cc[VTIME] = 0;
		tcflush(m_pointer, TCIFLUSH | TCIOFLUSH);
		tcsetattr(m_pointer, TCSANOW, &new_tio);
	}

	/**
	 * @brief Write a msg to serial port
	 *
	 * @param msg the message
	 * @return 1 on error, 0 if good
	 */
	int um25c_write(uint8_t msg)
	{
		fd_set wfds;
		FD_ZERO(&wfds);
		FD_SET(m_pointer, &wfds);
		int retv = select(m_pointer + 1, NULL, &wfds, NULL, NULL);
		if (retv == -1) {
			fprintf(stderr, "Failed to read from serial port: %s\n",
			        strerror(errno));
			return 1;
		}

		if (FD_ISSET(m_pointer, &wfds)) {
			write(m_pointer, &msg, 1);
			syncfs(m_pointer);
			return 0;
		}
		return 1;
	}

	/**
	 * Data format of UM25C
	 * https://sigrok.org/wiki/RDTech_UM_series
	 */
	typedef struct __UMC_MES {
		uint32_t milliamps;
		uint32_t milliwatts;
	} __attribute__((packed, aligned(1))) UMC_MES;

	/**
	 * Total 130 byte.
	 * Attributes enforces this.
	 */
	struct UMC {
		uint16_t unknown1;
		// 2
		uint16_t millivolts;
		// 4
		uint16_t tenths_milliamps;
		// 6
		uint32_t milliwatts;
		// 10
		uint16_t temp_celsius;
		// 12
		uint16_t temp_fahrenheit;
		// 14
		uint16_t current_datagroup;
		// 16
		UMC_MES mes[10];
		// 96
		uint16_t pline_centivolts;
		// 98
		uint16_t nline_centivolts;
		// 100
		uint16_t charge_mode;
		// 102
		uint32_t milliamps_threshold;
		// 106
		uint32_t milliwatts_threshold;
		// 110
		uint16_t current_threshold_centivolt;
		// 112
		uint32_t recording_time;
		// 116
		uint16_t recording_active;
		// 118
		uint16_t screen_timeout;
		// 120
		uint16_t screen_backlight;
		// 122
		uint32_t resistance_deciohm;
		// 126
		uint16_t current_screen;
		// 128
		uint16_t unknown2;
		// 130

	} __attribute__((packed, aligned(1)));

	typedef union {
		uint8_t raw[130];
		UMC umc;
	} UMC_READ;
	static void convert(UMC *umc)
	{
		umc->millivolts = ntohs(umc->millivolts);
		umc->tenths_milliamps = ntohs(umc->tenths_milliamps);
		umc->temp_celsius = ntohs(umc->temp_celsius);
		umc->temp_fahrenheit = ntohs(umc->temp_fahrenheit);
		umc->current_datagroup = ntohs(umc->current_datagroup);
		umc->pline_centivolts = ntohs(umc->pline_centivolts);
		umc->nline_centivolts = ntohs(umc->nline_centivolts);
		umc->charge_mode = ntohs(umc->charge_mode);
		umc->current_threshold_centivolt =
		    ntohs(umc->current_threshold_centivolt);
		umc->recording_active = ntohs(umc->recording_active);
		umc->screen_timeout = ntohs(umc->screen_timeout);
		umc->screen_backlight = ntohs(umc->screen_backlight);
		umc->current_screen = ntohs(umc->current_screen);
		for (int i = 0; i < 10; i++) {
			umc->mes[i].milliamps = ntohl(umc->mes[i].milliamps);
			umc->mes[i].milliwatts = ntohl(umc->mes[i].milliwatts);
		}
		umc->resistance_deciohm = ntohl(umc->resistance_deciohm);
		umc->milliwatts = ntohl(umc->milliwatts);
		umc->milliamps_threshold = ntohl(umc->milliamps_threshold);
		umc->milliwatts_threshold = ntohl(umc->milliwatts_threshold);
		umc->recording_time = ntohl(umc->recording_time);
	}

	/**
	 * @brief Returns a data sample from UM25C device
	 *
	 * @return um25c::UMC
	 */
	UMC get_data_sample()
	{
		UMC_READ umc;
		/**
		 * Request a data dump.
		 */
		if (um25c_write(msg_data_dump)) {
			throw std::runtime_error(
			    "Could not write message to bluetooth device!");
		}
		/**
		 * Read response
		 */
		ssize_t index = 0;
		while (index < data_dump_length) {
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(m_pointer, &rfds);
			int retv = select(m_pointer + 1, &rfds, NULL, NULL, NULL);
			if (retv == -1) {
				fprintf(stderr, "Failed to read from serial port: %s\n",
				        strerror(errno));
				break;
			}
			/** If there is data to read, read it. */
			if (FD_ISSET(m_pointer, &rfds)) {
				ssize_t r = read(m_pointer, &(umc.raw[index]), 130 - index);
				if (r < 0) {
					fprintf(stderr, "Failed top read from serial port: %s\n",
					        strerror(errno));
					break;
				}
				index += r;
			}
		}
		convert(&(umc.umc));
		return umc.umc;
	}
	typedef std::pair<UMC, std::chrono::steady_clock::time_point> timed_record;

	timed_record _get_data_sample_timed()
	{
		return {get_data_sample(), std::chrono::steady_clock::now()};
	}

	data get_data_sample_timed() override
	{
		auto rec = get_data_sample();
		return {std::chrono::steady_clock::now(), double(rec.millivolts),
		        double(rec.tenths_milliamps * 10), double(rec.milliwatts)};
	}

	~um25c()
	{
		tcflush(m_pointer, TCIFLUSH);
		tcsetattr(m_pointer, TCSANOW, &m_old_termios);
		close(m_pointer);
	}
};
}  // namespace Energy
