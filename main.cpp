// Copyright 2023 Northern.tech AS
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <iostream>

#include "libflash/fileio.hpp"
#include "libflash/optimized_writer.hpp"

#include <getopt.h>

void PrintHelp() {
	std::cout
		<< "Usage: mender-flash [-h|--help] [-s|--input-size <INPUT_SIZE>] -i|--input <INPUT_PATH> -o|--output <OUTPUT_PATH>"
		<< std::endl;
}

int main(int argc, char *argv[]) {
	int volumeSize = 0;
	std::string inputPath;
	std::string outputPath;
	const size_t blockSize = 1024 * 1024; // 1MB block size

	while (1) {
		static struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"input-size", required_argument, 0, 's'},
			{"input", required_argument, 0, 'i'},
			{"output", required_argument, 0, 'o'},
			{0, 0, 0, 0}};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hs:i:o:", long_options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'h':
			PrintHelp();
			return 0;

		case 's':
			volumeSize = atoi(optarg);
			break;

		case 'i':
			inputPath = optarg;
			break;

		case 'o':
			outputPath = optarg;
			break;

		case '?':
			break;

		default:
			PrintHelp();
			exit(EXIT_FAILURE);
		}
	}

	if (inputPath.empty() || outputPath.empty()) {
		std::cerr << "Wrong input parameters!" << std::endl;
		PrintHelp();
		exit(EXIT_FAILURE);
	}

	mender::io::File srcFile;
	mender::io::File dstFile;

	std::shared_ptr<mender::io::FileReader> reader;
	if (inputPath == "stdin") {
		srcFile = mender::io::GetInputStream();
		reader = std::make_shared<mender::io::InputStreamReader>();
	} else {
		auto src = mender::io::Open(inputPath);
		if (!src) {
			std::cerr << "Failed to open source: " << inputPath << " (" << src.error().message
					  << ")";
			exit(EXIT_FAILURE);
		}
		srcFile = src.value();
		reader = std::make_shared<mender::io::FileReader>(srcFile);
	}

	bool isUBI = false;
	auto isUBIRes = mender::io::IsUBIDevice(outputPath);
	if (isUBIRes.has_value()) {
		isUBI = isUBIRes.value();
	}

	auto dst = mender::io::Open(
		outputPath,
		!isUBI, // read: only for non-UBI volumes
		true);  // write

	if (!dst) {
		std::cerr << "Failed to open destination: " << outputPath << " (" << dst.error().message
				  << ")";
		exit(EXIT_FAILURE);
	}
	dstFile = dst.value();

	if (isUBI) {
		auto res = mender::io::SetUbiUpdateVolume(dstFile, volumeSize);
		if (res != mender::common::error::NoError) {
			std::cerr << res.message;
			exit(EXIT_FAILURE);
		}
	}

	mender::io::LimitedFlushingWriter flushWriter(dstFile, volumeSize, blockSize);
	mender::io::FileWriter writer(dstFile);
	mender::io::FileReadWriterSeeker readwriter(isUBI ? writer : flushWriter);
	mender::OptimizedWriter optWriter(*reader, readwriter, blockSize, volumeSize);
	optWriter.Copy(!isUBI);

	auto statistics = optWriter.GetStatistics();

	std::cout << "================ STATISTICS ================" << std::endl;
	std::cout << "Blocks written: " << statistics.mBlocksWritten << std::endl;
	std::cout << "Blocks omitted: " << statistics.mBlocksOmitted << std::endl;
	std::cout << "Bytes  written: " << statistics.mBytesWritten << std::endl;
	std::cout << "============================================" << std::endl;

	if (srcFile != mender::io::GetInputStream()) {
		mender::io::Close(srcFile);
	}
	mender::io::Close(dstFile);

	exit(EXIT_SUCCESS);
}
