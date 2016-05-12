//
// Created by user on 10/4/15.
//

#include "../flug/src/stdafx.h"
#include "AgilentOscope.h"
#include "../flug/src/helpers/JsonBson.h"

#define INFINIIUM_BUF_SIZE ((size_t)(1024 * 10))


namespace SciKit {
	AgilentOscope::AgilentOscope(const std::string & deviceInstanceName, const std::string & devType) :
	DeviceDriver (deviceInstanceName, devType) {
		m_connected = false;
	}

	AgilentOscope::~AgilentOscope() {
	}


	bool AgilentOscope::initModule() {
		try {
			m_sock.connect(m_addr, m_port);
			m_connected = true;
			return true;
		} catch (std::runtime_error &err) {
			std::cout << err.what() << std::endl;
			m_connected = false;
			return false;
		}
	}

	bool AgilentOscope::destroyModule() {
		return true;
	}

	double AgilentOscope::getFloating(const std::string &num) {
		std::stringstream ss;
		ss << num;
		double ret;
		ss >> ret;
		return ret;
	}

	void AgilentOscope::handleCommandRequest(Request &req, Json::Value & root) {
		bool res = command(req.m_json["request"].asString());
		if (!res) {
			root["status"] = "failure";
			root["description"] = "Can't perform command on Agilent device";
		} else {
			root["status"] = "success";
		}
	}

	void AgilentOscope::handleRequestRequest(Request &req, Json::Value &root) {
		std::string response;
		bool res = request(req.m_json["request"].asString(), response);
		if (!res) {
			root["status"] = "failure";
			root["description"] = "Can't perform request to Agilent device";
		} else {
			root["status"] = "success";
			root["value"] = response;
		}
	}

	void AgilentOscope::handleGetDataRequest(Request &req, Json::Value &root) {
		std::cout << "Started handling the getData() request" << std::endl;

		/*std::cout << "Updating variables" << std::endl;

		for (int i = 0; i < req.m_json["controls"].size(); i++) {
			std::string newVal;
			tryUpdateVariable(req.m_json["controls"][i]["cmd"].asString(),
							  req.m_json["controls"][i]["req"].asString(),
							  req.m_json["controls"][i]["val"].asString(),
							  newVal);
			root["controls"][req.m_json["controls"][i]["id"].asString()] = newVal;
		}

		std::cout << "Finished updating variables" << std::endl;*/


		command(":SYSTEM:HEADER OFF");
		command(":ACQUIRE:MODE RTIME");
		command(":ACQUIRE:POINTS:AUTO OFF");
		command(":ACQUIRE:AVERAGE OFF");
		command(":ACQUIRE:INTERPOLATE OFF");
		command(":ACQUIRE:POINTS 500");
		if (m_oldModel) {
		    command(":WAVEFORM:UNSIGNED OFF");
		}
		command(":WAVEFORM:BYTEORDER LSBFIRST");
		command(":WAVEFORM:FORMAT WORD");

		std::string digCmd = ":DIGITIZE ";
		for (int i = 1; i <= 4; i++) {
			std::stringstream ss;
			ss << i;
			std::string chno = ss.str();
			if (m_oldModel) {
			    command(":CHAN" + chno + ":IMP FIFTY");
			}
			if (req.m_json["channels"][i-1].asString() == "true") {
				digCmd += "CHANNEL"+chno+",";
			}
		}
		command(digCmd);

		for (int i = 1; i <= 4; i++) {
			if (req.m_json["channels"][i-1].asString() == "true") {
				std::vector<int16_t> data;

				std::stringstream ss;
				ss << i;
				std::string chno = ss.str();

				command(":WAVEFORM:SOURCE CHAN" + chno);
				std::string yIncrStr, yOriginStr, xIncrStr;
				request(":WAVEFORM:YINCREMENT?", yIncrStr);
				request(":WAVEFORM:XINCREMENT?", xIncrStr);
				request(":WAVEFORM:YORIGIN?", yOriginStr);
				double yIncr = getFloating(yIncrStr);
				double xIncr = getFloating(xIncrStr);
				double yOrigin = getFloating(yOriginStr);
				commandUnsafe(":WAVEFORM:DATA?");
				std::cout << "Sent the configuration, recieving data.." << std::endl;
				try {
					getWordData(data);
				} catch (std::runtime_error &err) {
					std::cout << err.what() << std::endl;
					root["status"] = "error";
					return;
				}
				std::cout << "Data recieved, putting to JSON: " << data.size() << " elements" << std::endl;


				for (int j = 0; j < data.size(); j++) {
					root["data"][i-1][j] = data[j] * yIncr + yOrigin;
				}

				root["data_size"] = (int) data.size();
				root["xincr"] = xIncr;

				command(":CHAN" + chno + ":DISPLAY ON");
			} else {
                root["data"][i-1] = (Json::Value) JsonBson(std::string("[]"));
            }
		}
		command(":RUN");

		root["status"] = "success";

	}

	bool AgilentOscope::handleRequest(Request &req, Response &resp) {
		Json::Value root;
		std::string reqtype = req.m_json["reqtype"].asString();

		Json::StyledWriter writer;
		std::cout << std::endl << writer.write(req.m_json) << std::endl;

		if (reqtype == "command") {
			handleCommandRequest(req, root);
		} else if (reqtype == "request") {
			handleRequestRequest(req, root);
		} else if (reqtype == "getData") {
			handleGetDataRequest(req, root);
		} else if (reqtype == "getDeviceInfo") {
			handleIsOnlineRequest(req, root);
		}

		resp = root;
		return true;
	}


	void AgilentOscope::handleIsOnlineRequest(Request &req, Json::Value &root) {
		std::string idn = "";
		if (m_connected) {
			request("*IDN?", idn);
		}

		root["deviceType"] = "AgilentOscope";
		if (idn != "") {
			root["deviceName"] = m_deviceName;
			root["deviceType"] = m_deviceType;
			root["deviceStatus"] = "online";
		} else {
			root["deviceStatus"] = "offline";
		}
		root["status"] = "success";
	}

	void AgilentOscope::tryUpdateVariable(const std::string &cmd, const std::string &req,
										  const std::string &val, std::string &newVal) {

		command(cmd+' '+val);
		request(req, newVal);
	}

	bool AgilentOscope::command(const std::string &str) {
		std::cout << str << std::endl;
		if (str.find("?") != std::string::npos) {
			throw std::runtime_error("Command string contains '?' mark. That's bullshit.");
		}
		try {
			m_sock.sendLine(str);
			return true;
		} catch (std::runtime_error &err) {
			m_connected = false;
			std::cout << err.what() << std::endl;
			return false;
		}
	}

	bool AgilentOscope::commandUnsafe(const std::string &str) {
		std::cout << str << std::endl;
		try {
			m_sock.sendLine(str);
			return true;
		} catch (std::runtime_error &err) {
			m_connected = false;
			std::cout << err.what() << std::endl;
			return false;
		}
	}

	bool AgilentOscope::request(const std::string &req, std::string &resp) {
		std::cout << req << std::endl;

		if (req.length() == 0 || req[req.length() - 1] != '?') {
			std::cout << "Request is missterminated or zero-sized" << std::endl;
			return false;
		}

		try {
			m_sock.sendLine(req);
		} catch (std::runtime_error &err) {
			m_connected = false;
			std::cout << err.what() << std::endl;
			return false;
		}

		std::cout << "Getting repsonse" << std::endl;

		try {
			m_sock.recvLine(resp);
		} catch (std::runtime_error & err) {
			m_connected = false;
			std::cout << err.what() << std::endl;
			return false;
		}

		std::cout << resp << std::endl;
		return true;
	}


	size_t AgilentOscope::parseWordRespHeader(const char *buf,
											  size_t &dataStart) {
		std::cout << "Data response header [" << std::string(buf, buf + 10) << "]" << std::endl;
		if (buf[0] != '#') {
			throw std::runtime_error("InfiniiumConnection::parseWordRespHeader: \
					invalid header");
		}
		size_t digitsCount = (size_t) (buf[1] - '0');
		if (digitsCount < 0 || digitsCount > 9) {
			throw std::runtime_error("InfiniiumConnection::parseWordRespHeader: \
					wrong digitsCount");
		}

		size_t dataSize = 0;

		for (size_t i = 0; i < digitsCount; i++) {
			size_t d = (size_t) (buf[2 + i] - '0');
			if (d < 0 || d > 9) {
				throw std::runtime_error("InfiniiumConnection::parseWordRespHeader: \
						wrong digit");
			}
			dataSize = dataSize * 10 + d;
		}

		dataStart = 2 + digitsCount;

		std::cout << "Data count " << dataSize << std::endl;
		return dataSize;

	}

	void AgilentOscope::parseWordsFromBuf(const char *buf, size_t size,
										  std::vector<int16_t> &data) {
		size_t i;
		size_t reminder = size % 2;
		size_t count = size / 2;
		const int16_t *ptr = reinterpret_cast<const int16_t *>(buf);
		data.assign(ptr, ptr + count);
	}

	void AgilentOscope::getWordData(std::vector<int16_t> &data) {
		uint8_t buf[INFINIIUM_BUF_SIZE];

		size_t dataStart;
		size_t recieved = m_sock.recv(buf, INFINIIUM_BUF_SIZE);
		size_t dataLength = parseWordRespHeader(reinterpret_cast<const char *>(buf), dataStart);
		size_t fullLength = dataStart + dataLength + 1;

		data.clear();

		std::vector<char> dataBuf(dataLength + 1);
		dataBuf.assign(buf + dataStart, buf + recieved);

		while (recieved < fullLength) {
			size_t ret = m_sock.recv(reinterpret_cast<uint8_t*>(dataBuf.data() + recieved - dataStart),
									 fullLength - recieved);
			recieved += ret;
		}

		parseWordsFromBuf(dataBuf.data(), dataLength, data);

		if (dataBuf[dataLength] != '\n') {
			throw std::runtime_error("InfiniiumConnection::getWordData(): wrong message ending");
		}
	}


	bool AgilentOscope::loadConfig(Json::Value & config) {
        m_port = config["port"].asString();
        m_addr = config["addr"].asString();
		if (config.isMember("old")) {
		    m_oldModel = config["old"].asBool();
		}

		return true;
	}

    Module::State AgilentOscope::getState() {
        return ST_ONLINE;
    }


}
