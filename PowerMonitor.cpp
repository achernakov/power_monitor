//
// Created by user on 12.05.16.
//

#include "../flug/src/stdafx.h"
#include "PowerMonitor.h"
#include "../flug/src/helpers/JsonBson.h"
#include "../flug/src/kernel/scripts/OctaveBuiltin.h"

FLUG_DYNAMIC_DRIVER(SciKit::PowerMonitor);

namespace SciKit {
    PowerMonitor::PowerMonitor(const std::string & deviceInstanceName, const std::string & devType) :
            AgilentOscope (deviceInstanceName, devType) {
    }

    PowerMonitor::~PowerMonitor() {

    }

    bool PowerMonitor::handleRequest(Request &req, Response &resp) {
        std::string reqtype = req.m_json["reqtype"].asString();
        if (reqtype == "getPowerPoint") {
            return handleGetPowerPoint(req, resp);
        } else
            return AgilentOscope::handleRequest(req, resp);
    }

    bool PowerMonitor::initModule() {
        return AgilentOscope::initModule();
    }

    bool PowerMonitor::destroyModule() {
        return AgilentOscope::destroyModule();
    }

    bool PowerMonitor::loadConfig(Json::Value &config) {
        return AgilentOscope::loadConfig(config);
    }

    Module::State PowerMonitor::getState() {
        return AgilentOscope::getState();
    }



    bool PowerMonitor::handleGetPowerPoint(Request &req, Response &resp) {
        Json::Value root;
        OctaveBuiltin test;
        Json::Value in, out;

        std::vector<double> fwd, ref;
        double xincr;

        getWaves("1", "2", fwd, ref, xincr);

        for (int i = 0; i < fwd.size(); i++) {
            root["data"]["ref"][i] = ref[i];
            root["data"]["fwd"][i] = fwd[i];
            in["data"]["ref"][i] = ref[i];
            in["data"]["fwd"][i] = fwd[i];
        }
        root["data"]["xincr"] = xincr;
        in["data"]["xincr"] = xincr;

        test.runScript("analyze", in, out);
        root["out"] = out;


        root["status"] = "success";
        resp = root;
        return true;
    }

    static void intArrayToDoubleArray (const std::vector<int16_t> & in,
                                       std::vector<double> & out,
                                       double mul, double add) {
        out.clear();
        for (auto v: in) {
            out.push_back(v * mul + add);
        }
    }


    void PowerMonitor::getWaves(const std::string & fwdChan, const std::string & refChan,
                               std::vector<double> & fwd, std::vector<double> & ref,
                                double & xincr) {

        std::stringstream ss1, ss2;
        std::string acqRangeStringStr = "1.00E-6";
        std::string acqSamplingRateStr = "1.00E+10";
        double acqRange, acqSRate;
        ss1 << acqRangeStringStr;
        ss1 >> acqRange;
        ss2 << acqSamplingRateStr;
        ss2 >> acqSRate;

        //command("*RST");
        //command(":AUTOSCALE");
        command(":SYSTEM:HEADER OFF");

        command(":ACQUIRE:MODE RTIME");
        command(":ACQUIRE:INTERPOLATE OFF");
        command(":ACQUIRE:AVERAGE OFF");

        command(std::string(":TIMEBASE:RANGE ") +
                             acqRangeStringStr);

        command(":ACQUIRE:POINTS:AUTO ON");
        command(":ACQUIRE:SRATE:AUTO OFF");
        command(std::string(":ACQUIRE:SRATE ") +
                             acqSamplingRateStr);

        command(":WAVEFORM:BYTEORDER LSBFIRST");
        command(":WAVEFORM:FORMAT WORD");
        command(":DIGITIZE CHANNEL"+fwdChan+",CHANNEL"+refChan);

        std::vector<int16_t> data;


        for (size_t chanNo = 1; chanNo <= 4; chanNo++) {
            std::stringstream ss;
            ss << chanNo;
            std::string no, chSelector(":WAVEFORM:SOURCE CHANNEL");
            ss >> no;

            double yIncr, yOrigin, timeScale;
            if (chanNo  == atoi(fwdChan.c_str()) || chanNo == atoi(refChan.c_str())) {
                command((chSelector + no).c_str());
                commandUnsafe(":WAVEFORM:DATA?");
                getWordData(data);
                data.erase(data.begin()+(int)(acqRange * acqSRate), data.end());
                getTypedParam(":WAVEFORM:YINCREMENT?", yIncr);
                getTypedParam(":WAVEFORM:YORIGIN?", yOrigin);
                getTypedParam(":WAVEFORM:XINCREMENT?", timeScale);
                xincr = timeScale;
                //writeDataToFile(fileSelector + no + fileExtension, data, yIncr, yOrigin, timeScale);
            }
            if (chanNo  == atoi(fwdChan.c_str())) {
                intArrayToDoubleArray(data, fwd, yIncr, yOrigin);
            }
            if (chanNo  == atoi(refChan.c_str())) {
                intArrayToDoubleArray(data, ref, yIncr, yOrigin);
            }
        }


    }


}