//
// Created by user on 12.05.16.
//

#ifndef POWER_MONITOR_POWERMONITOR_H
#define POWER_MONITOR_POWERMONITOR_H

#include "AgilentOscope.h"


namespace SciKit {
    class PowerMonitor : public AgilentOscope {

    public:
        PowerMonitor () = delete;
        PowerMonitor (const std::string & deviceInstanceName, const std::string & devType);
        virtual ~PowerMonitor ();

        virtual bool initModule ();
        virtual bool destroyModule ();
        virtual bool handleRequest (Request & req, Response & resp);
        virtual bool loadConfig (Json::Value & config);
        virtual State getState ();


    protected:
        void getWaves (const std::string & fwdChan, const std::string & refChan,
                       std::vector<double> & fwd, std::vector<double> & ref,
                       double & xincr);
        void getWavesDummy (const std::string & fwdChan, const std::string & refChan,
                       std::vector<double> & fwd, std::vector<double> & ref,
                       double & xincr);
        bool handleGetPowerPoint (Request & req, Response & resp);


        template<typename T> void getTypedParam(const std::string & cmd, T & response) {
            std::string strResp;
            request (cmd, strResp);
            std::stringstream buff(strResp);
            buff >> response;
        }

    private:
        float m_baseFreq;
        Json::Value m_sMatrix;
        Json::Value m_att1;
        Json::Value m_att2;
        Json::Value m_cab1;
        Json::Value m_cab2;
        Json::Value m_portAssignment;
        bool m_fake;
    };
}


#endif //POWER_MONITOR_POWERMONITOR_H
