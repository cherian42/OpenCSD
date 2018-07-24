/*
 * pktdecoder.h
 *
 *  Created on: Jul 13, 2018
 *      Author: dku
 */

#ifndef SRC_PKTDECODER_H_
#define SRC_PKTDECODER_H_
//

class pkt_decoder {
public:
	pkt_decoder();
	void initLogging();
	void initSS(std::string);
	void initDecoding(bool decode);
	void doDecode(uint8_t* pbuf, int32_t bufSize);
	void writeCFG( std::string cfgFile);
	void cleanup();
private:
	ocsdMsgLogger logger;
	int logOpts;
	std::string logfileName ;
	bool outRawPacked ;
	bool outRawUnpacked ;
	bool ss_verbose ;
	bool decode ;
	bool no_undecoded_packets ;
	bool pkt_mon ;
	int test_waits ;
	bool all_source_ids;
	std::vector<uint8_t> id_list;

	std::string cfgFile;
//	std::string ss_path;
	SnapShotReader ss_reader;
	std::string source_buffer_name;
	ocsdDefaultErrorLogger err_log;

    CreateDcdTreeFromSnapShot tree_creator;
    // pointers for the printers
    // for printing the raw frames; invoked after Frame-demux stage
    RawFramePrinter *pFramePrinter ;
    // for printing the decoded frames - invoked after decode stage
    TrcGenericElementPrinter *pGenElemPrinter;

	std::vector<ItemPrinter *> printers;

	void AttachPacketPrinters( DecodeTree *dcd_tree);
	void ConfigureFrameDeMux(DecodeTree *dcd_tree, RawFramePrinter **framePrinter);
    bool element_filtered(uint8_t elemID);
    uint32_t trace_index;
    uint32_t count;
    std::ostringstream m_oss;
    void dumpLog();

};

#endif /* SRC_PKTDECODER_H_ */
