#/bin/sh


export LD_LIBRARY_PATH=$PATH:./tests/bin/builddir/
# run the pktdecoder test; results are stored in cfg.txt
./tests/bin/builddir/pktdecoder_test -ss_dir mytraces/ss/posix_lwip/
## count the number of unique BBs in the decoded CFG file
cat cfg.txt | sort | uniq -c
