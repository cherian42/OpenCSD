/*
 * pktdecoder.cpp
 *
 *  Created on: Jul 13, 2018
 *      Author: dku
 */

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <assert.h>

#include "opencsd.h"
#include "trace_snapshots.h"
#include "pktdecoder.h"
using namespace std;


pkt_decoder::pkt_decoder() {

	logOpts = ocsdMsgLogger::OUT_NONE;
	logfileName = "pkt_decoder.ppl";
	// to print the raw frames; before decoding
	outRawPacked = false;
	outRawUnpacked = false;
	ss_verbose = false;
	decode = false;
	no_undecoded_packets = false;
	pkt_mon = false;
	test_waits = 0;
	source_buffer_name = "";
	all_source_ids = true;
	trace_index = 0;
	count = 0;
	pGenElemPrinter = NULL;
	pFramePrinter = NULL;

}

void pkt_decoder::initLogging() {

    logOpts =  ocsdMsgLogger::OUT_FILE | ocsdMsgLogger::OUT_STDOUT;
	// setup the logger
    logger.setLogOpts(logOpts);
    logger.setLogFileName(logfileName.c_str());

	// setup the error logging level
    err_log.initErrorLogger(OCSD_ERR_SEV_INFO);
    err_log.setOutputLogger(&logger);
}

void pkt_decoder::initSS(std::string ss_path) {

    bool ret;

    m_oss << "Trace Packet Lister : reading snapshot from path " << ss_path << "\n";
    dumpLog();

    ss_reader.setSnapshotDir(ss_path);
    ss_reader.setErrorLogger(&err_log);
    ss_reader.setVerboseOutput(ss_verbose);

    ret = ss_reader.snapshotFound();
    assert(ret == true);

    ret = ss_reader.readSnapShot();
    assert(ret == true);

    vector<std::string> sourceBuffList;
    ret = ss_reader.getSourceBufferNameList(sourceBuffList);

	if (ret) {
		bool bValidSourceName = false;
		// check source name list
		if (source_buffer_name.size() == 0) {
			// default to first in the list
			source_buffer_name = sourceBuffList[0];
			bValidSourceName = true;
		} else {
			for (size_t i = 0; i < sourceBuffList.size(); i++) {
				if (sourceBuffList[i] == source_buffer_name) {
					bValidSourceName = true;
					break;
				}
			}
		}
		if (bValidSourceName) {
			m_oss << "Using " << source_buffer_name << " as trace source\n";
		    dumpLog();

		} else {

			m_oss << "[ERROR]: Trace Packet Lister : Trace source name "
					<< source_buffer_name << " not found\n";
			m_oss << "Valid source names are:-\n";
			for (size_t i = 0; i < sourceBuffList.size(); i++) {
				m_oss << sourceBuffList[i] << "\n";
			}
			dumpLog();
			// assert always
			assert(false);
		}
	} else {
		m_oss << "[ERROR]: getSourceBufferNameList failed!! \n";
	    dumpLog();
		assert(false);
	}

}

void pkt_decoder::initDecoding(bool decode) {

	bool bPacketProcOnly = (decode == false);

	// create tree from the snapshot object
	tree_creator.initialise(&ss_reader, &err_log);
	bool ret = false;
	ret = tree_creator.createDecodeTree(source_buffer_name, bPacketProcOnly);
	assert(ret == true);

	// bPacketProcOnly == true ==> stop at the second stage - packet processing
	// decode == true ==> continue to the third stage - packet decoding.

	DecodeTree *dcd_tree = tree_creator.getDecodeTree();
	// set error logger for the decode tree
    dcd_tree->setAlternateErrorLogger(&err_log);

	// for monitor/sink
	AttachPacketPrinters(dcd_tree);

    ConfigureFrameDeMux(dcd_tree, &pFramePrinter);

    if(decode) {
        dcd_tree->addGenElemPrinter(&pGenElemPrinter);
    }

	// check if we have attached at least one printer
    assert(PktPrinterFact::numPrinters(dcd_tree->getPrinterList()) > 0);

    // no filtering based on source IDs
    // all_source_ids = true
	dcd_tree->clearIDFilter();
}
// true if element ID filtered out
bool pkt_decoder::element_filtered(uint8_t elemID)
{
    bool filtered = false;
    if(!all_source_ids)
    {
        filtered = true;
        std::vector<uint8_t>::const_iterator it;
        it = id_list.begin();
        while((it != id_list.end()) && filtered)
        {
            if(*it == elemID)
                filtered = false;
            it++;
        }
    }
    return filtered;
}

void pkt_decoder::AttachPacketPrinters( DecodeTree *dcd_tree)
{
    uint8_t elemID;
    std::ostringstream oss;

    // attach packet printers to each trace source in the tree
    DecodeTreeElement *pElement = dcd_tree->getFirstElement(elemID);
    while(pElement && !no_undecoded_packets)
    {
        if(!element_filtered(elemID))
        {
            oss.str("");

            ItemPrinter *pPrinter;
            ocsd_err_t err = dcd_tree->addPacketPrinter(elemID, (bool)(decode || pkt_mon),&pPrinter);
            if (err == OCSD_OK)
            {
                // if not decoding or monitor only
                if((!(decode || pkt_mon)) && test_waits)
                    pPrinter->setTestWaits(test_waits);

                oss << "Trace Packet Lister : Protocol printer " << pElement->getDecoderTypeName() <<  " on Trace ID 0x" << std::hex << (uint32_t)elemID << "\n";
            }
            else
                oss << "Trace Packet Lister : Failed to Protocol printer " << pElement->getDecoderTypeName() << " on Trace ID 0x" << std::hex << (uint32_t)elemID << "\n";
            logger.LogMsg(oss.str());

        }
        pElement = dcd_tree->getNextElement(elemID);
    }

}

void pkt_decoder::ConfigureFrameDeMux(DecodeTree *dcd_tree, RawFramePrinter **framePrinter)
{
    // configure the frame deformatter, and attach a frame printer to the frame deformatter if needed
    TraceFormatterFrameDecoder *pDeformatter = dcd_tree->getFrameDeformatter();
    if(pDeformatter != 0)
    {
        // configuration - memory alinged buffer
        uint32_t configFlags = OCSD_DFRMTR_FRAME_MEM_ALIGN;

        pDeformatter->Configure(configFlags);
        if (outRawPacked || outRawUnpacked)
        {
            if (outRawPacked) configFlags |= OCSD_DFRMTR_PACKED_RAW_OUT;
            if (outRawUnpacked) configFlags |= OCSD_DFRMTR_UNPACKED_RAW_OUT;
            dcd_tree->addRawFramePrinter(framePrinter, configFlags);
        }
    }
}


void pkt_decoder::writeCFG( std::string cfgFile)
{
	pGenElemPrinter->writeCFG(cfgFile.c_str());
}

void pkt_decoder::doDecode(uint8_t* pbuf, int32_t bufSize) {

	ocsd_datapath_resp_t dataPathResp = OCSD_RESP_CONT;
//	uint32_t trace_index = 0;     // index into the overall trace buffer (file).
	std::streamsize nBuffRead = bufSize;    // get count of data loaded.
	std::streamsize nTotalBytesProcessed = 0;      // amount processed in this buffer.
	uint32_t nUsedThisTime = 0;
	std::streamsize nBuffRemaining = nBuffRead;

    DecodeTree *dcd_tree = tree_creator.getDecodeTree();
    count = 0;
    // BUG: resetting trace_index results in
    //      failure to decode the second sample onwards
   // trace_index = 0;

	pGenElemPrinter->resetSessionStat();

    // process the current buffer load until buffer done, or fatal error occurs
    while((nTotalBytesProcessed < nBuffRead) && !OCSD_DATA_RESP_IS_FATAL(dataPathResp))
    {
        if(OCSD_DATA_RESP_IS_CONT(dataPathResp))
        {
            dataPathResp = dcd_tree->TraceDataIn(
                OCSD_OP_DATA,
                trace_index,
                (uint32_t)(nBuffRemaining),
				(pbuf+nTotalBytesProcessed),
                &nUsedThisTime);

            nTotalBytesProcessed += nUsedThisTime;
            trace_index += nUsedThisTime;
            nBuffRemaining = nBuffRead - nTotalBytesProcessed;

            m_oss <<" ["<<count<<"] "<< "dataPathResp: " << dataPathResp << " ";
            m_oss << "nUsedThisTime: " << nUsedThisTime << " ";
            m_oss << "nBuffProcessed: " << nTotalBytesProcessed << " ";
            m_oss << "nBuffRemaining: " << nBuffRemaining << "trace_index "<< trace_index<< "\n";
            dumpLog();
            ++count;

        }
        else // last response was _WAIT
        {
            m_oss <<" DP respo!=cont: "<<"\n";
            m_oss <<" ["<<count<<"] "<< "dataPathResp: " << dataPathResp << " ";
            m_oss << "nUsedThisTime: " << nUsedThisTime << " ";
            m_oss << "nBuffProcessed: " << nTotalBytesProcessed << " ";
            m_oss << "nBuffRemaining: " << nBuffRemaining << "trace_index "<< trace_index<< "\n";
            dumpLog();

        }
    }
	pGenElemPrinter->printSessionStat();

    // TODO: find the right sequence to correctly reset the decode tree to
    //       decode the next sample onwards

   // dataPathResp = dcd_tree->TraceDataIn(OCSD_OP_FLUSH,0,0,0,0);
   // dataPathResp = dcd_tree->TraceDataIn(OCSD_OP_EOT,0,0,0,0);
   //  dataPathResp = dcd_tree->TraceDataIn(OCSD_OP_RESET,0,0,0,0);
}

void pkt_decoder::cleanup() {

	// get rid of the decode tree.
	tree_creator.destroyDecodeTree();

	// get rid of all the printers.
	std::vector<ItemPrinter *>::iterator it;
	it = printers.begin();
	while (it != printers.end()) {
		delete *it;
		it++;
	}
	printers.clear();
}

void pkt_decoder::dumpLog() {

	logger.LogMsg(m_oss.str());
	// empty the stream
	m_oss.str(std::string());
	// clear any errors in the stream
	m_oss.clear();
}



