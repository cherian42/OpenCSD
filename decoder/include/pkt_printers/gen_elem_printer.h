/*
 * \file       gen_elem_printer.h
 * \brief      OpenCSD : Generic element printer class.
 * 
 * \copyright  Copyright (c) 2015, ARM Limited. All Rights Reserved.
 */

/* 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 * may be used to endorse or promote products derived from this software without 
 * specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */ 
#ifndef ARM_GEN_ELEM_PRINTER_H_INCLUDED
#define ARM_GEN_ELEM_PRINTER_H_INCLUDED

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <assert.h>

#include "opencsd.h"

class TrcGenericElementPrinter : public ItemPrinter, public ITrcGenElemIn
{
public:
    TrcGenericElementPrinter();
    virtual ~TrcGenericElementPrinter() {};

    virtual ocsd_datapath_resp_t TraceElemIn(const ocsd_trc_index_t index_sop,
                                              const uint8_t trc_chan_id,         
                                              const OcsdTraceElement &elem);

    // funtionality to test wait / flush mechanism
    void ackWait() { m_needWaitAck = false; };
    const bool needAckWait() const { return m_needWaitAck; };
    void writeCFG(const char* cfgFileName);
    void printSessionStat();
    void resetSessionStat();

protected:
    bool m_needWaitAck;
private:
    typedef struct {
    	uint64_t start;
    	uint64_t end;
    }instruction_range_t;
    typedef std::vector<instruction_range_t> cfg_db_t;
    cfg_db_t cfg_db;
    uint64_t sesionBBCount;


};


inline TrcGenericElementPrinter::TrcGenericElementPrinter() :
    m_needWaitAck(false)
{
}

inline ocsd_datapath_resp_t TrcGenericElementPrinter::TraceElemIn(const ocsd_trc_index_t index_sop,
                                              const uint8_t trc_chan_id,
                                              const OcsdTraceElement &elem)
{
    ocsd_datapath_resp_t resp = OCSD_RESP_CONT;

    std::string elemStr;
    std::ostringstream oss;
#if 0
    oss << "Idx:" << index_sop << "; ID:"<< std::hex << (uint32_t)trc_chan_id << "; ";
    elem.toString(elemStr);
    oss << elemStr << std::endl;
    itemPrintLine(oss.str());
#endif

	switch (elem.getType()) {
	case OCSD_GEN_TRC_ELEM_INSTR_RANGE:
		cfg_db.push_back({elem.st_addr, elem.en_addr});
		sesionBBCount++;

		break;
	case OCSD_GEN_TRC_ELEM_EXCEPTION:
		cfg_db.push_back({0xdead, 0xdead});
		sesionBBCount++;

		break;
	case OCSD_GEN_TRC_ELEM_EXCEPTION_RET:
		cfg_db.push_back({0xbeef, 0xbeef});
		sesionBBCount++;

		break;
	default:
		break;
	}


    // funtionality to test wait / flush mechanism
    if(m_needWaitAck)
    {
        oss.str("");
        oss << "WARNING: Generic Element Printer; New element without previous _WAIT acknowledged\n";
        itemPrintLine(oss.str());
        m_needWaitAck = false;
    }
    
    if(getTestWaits())
    {
        resp = OCSD_RESP_WAIT; // return _WAIT for the 1st N packets.
        decTestWaits();
        m_needWaitAck = true;
    }
    return resp; 
}
inline void TrcGenericElementPrinter::writeCFG(
    const char* cfgFileName)
{

    std::cout << "======== writeCFG" << std::endl;
    std::fstream cfg(cfgFileName, std::ofstream::out);
    assert(cfg.is_open());
    cfg_db_t::iterator itr = cfg_db.begin();

    for (; itr != cfg_db.end(); ++itr) {
        cfg << std::hex << itr->start << "," << std::hex << itr->end
                << std::endl;
    }

    cfg.close();
    cfg_db.clear();

}
inline void TrcGenericElementPrinter::printSessionStat() {
    std::cout << "cfg_db.size: "<<  cfg_db.size()<< std::endl;
    std::cout << "sesionBBCount: "<<  sesionBBCount<< std::endl;

}
inline void TrcGenericElementPrinter::resetSessionStat(){
	sesionBBCount=0;
}


#endif // ARM_GEN_ELEM_PRINTER_H_INCLUDED

/* End of File gen_elem_printer.h */
