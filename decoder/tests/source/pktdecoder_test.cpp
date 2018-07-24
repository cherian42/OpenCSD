/*
 * pktdecoder_test.cpp
 *
 *  Created on: Jul 13, 2018
 *      Author: dku
 */
#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <assert.h>
#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>

 // the snapshot reading test library
#include "trace_snapshots.h"
#include <pktdecoder.h>

static pkt_decoder mPktDecoder;
static std::string mSSPath = "./";
static std::string mCFGFile = "/cfg.txt";
static bool mDecode = true;
//static unsigned int mIteration = 59;
static std::ifstream mTraceFileStream;

using namespace std;
void testLosslessTracing();
void initTraceFileStream();
void closeTraceFileStream();
bool process_cmd_line_opts(int argc, char* argv[]);

int main(
    int argc,
    char* argv[])
{

	process_cmd_line_opts(argc, argv);

    mPktDecoder.initLogging();
    mPktDecoder.initSS(mSSPath);
    mPktDecoder.initDecoding(mDecode);

    initTraceFileStream();

	testLosslessTracing();

	closeTraceFileStream();

}

void testLosslessTracing() {
	cout << " [tester] testing lossless tracing \n";

	// sample size of 64KB
	static const int sampleSize = 1024 * 64;
	// temp buffer to store the trace sample
	uint8_t trace_buffer[sampleSize];
	streamsize traceBufSize = 0;
	bool proceed = true;
	uint32_t itr = 0;

	// assert that the accumulated trace file is opened
	assert(mTraceFileStream.is_open());

	while(proceed==true) {

		// read 64KB sample from the accumulated trace file
		mTraceFileStream.read((char *)&trace_buffer[0], sampleSize);
		traceBufSize = mTraceFileStream.gcount();

		if(mTraceFileStream.eof()) {
			proceed = false;
			cout << " [" << itr <<"] reached EOF \n";

		}
		cout << "[" << itr <<"] sending "  << traceBufSize << " bytes for decoding\n";
		// decode the 64KB sample
		mPktDecoder.doDecode(&trace_buffer[0], traceBufSize);

		++itr;

	}


	static std::string cfgFileName_temp = "cfg.txt";
	mPktDecoder.writeCFG(cfgFileName_temp);
	mPktDecoder.cleanup();

}


void initTraceFileStream(){
	std::string cstrace = mSSPath;
	cstrace.append("/cstrace.bin");
	cout<< "[tester] opening trace bin: "<< cstrace << endl;

	mTraceFileStream.open(cstrace,std::ifstream::in | std::ifstream::binary);
	assert(mTraceFileStream.is_open());
}

void closeTraceFileStream(){
	assert(mTraceFileStream.is_open());
	mTraceFileStream.close();

}

typedef enum {
		ss_dir,
		decode,
		decode_only,
		iteration,
		argStrings_count
}argStrings_t;

static std::map<std::string, argStrings_t > argMap = {
		{"-ss_dir", ss_dir },
		{"-decode", decode },
		{"-decode_only", decode_only },
		{"-iteration", iteration },

};

bool process_cmd_line_opts(int argc, char* argv[])
{
    bool bOptsOK = true;
    std::string opt;
    if(argc > 1)
    {
        int options_to_process = argc - 1;
        int optIdx = 1;
        while((options_to_process > 0) && bOptsOK)
        {
            opt = argv[optIdx];
			switch (argMap[opt]) {
			case ss_dir:
				printf( "[tester] -ss_dir option.. ");
				options_to_process--;
				optIdx++;
				if (options_to_process) {
					mSSPath = argv[optIdx];
					cout<< mSSPath << endl;
				}
				else {
					printf(
							"[tester] Error: Missing directory string on -ss_dir option\n");
					bOptsOK = false;
				}
				break;
			default:
				printf(
						"[tester] Error: option not supported\n");
				bOptsOK = false;

			}
            options_to_process--;
            optIdx++;
        }

    }
    return bOptsOK;
}

