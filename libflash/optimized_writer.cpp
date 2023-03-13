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

#include <optimized_writer.hpp>
#include <iostream>

using namespace mender;

OptimizedWriter::OptimizedWriter(
	io::FileReader &reader, io::FileReadWriterSeeker &writer, size_t blockSize, size_t volumeSize) :
	blockSize_(blockSize),
	reader_(reader),
	readWriter_(writer),
	volumeSize_(volumeSize) {
}

Error OptimizedWriter::Copy(bool optimized) {
	statistics_.blocksWritten_ = 0;
	statistics_.blocksOmitted_ = 0;
	statistics_.bytesWritten_ = 0;

	io::Bytes rv(blockSize_);
	io::Bytes wv(blockSize_);

	while (true) {
		auto pos = reader_.Tell();
		if (!pos) {
			return pos.error();
		}
		auto position = pos.value();

		if (volumeSize_ && ((position + blockSize_) > volumeSize_)) {
			return NoError;
		}

		auto result = reader_.Read(rv.begin(), rv.end());
		if (!result) {
			return result.error();
		} else if (result.value() == 0) {
			return NoError;
		} else if (result.value() > rv.size()) {
			return mender::common::error::MakeError(
				mender::common::error::ProgrammingError,
				"Read returned more bytes than requested. This is a bug in the Read function.");
		}

		auto readBytes = result.value();

		if (NoError != readWriter_.SeekSet(position)) {
			return Error(
				std::error_condition(std::errc::io_error),
				"Failed to set seek on the destination file");
		}

		bool skipWriting = false;
		if (optimized) {
			auto readResult = readWriter_.Read(wv.begin(), wv.begin() + readBytes);
			if (readResult && readResult.value() == readBytes) {
				wv.resize(readResult.value());
				skipWriting = std::equal(rv.begin(), rv.end(), wv.data());
				if (skipWriting) {
					++statistics_.blocksOmitted_;
				}
			}
#if 0
			else if (!readResult) {
				return readResult.error();
			} else {
				return Error(std::error_condition(std::errc::io_error), "Short read of the output data block");
			}
#endif
		}

		if (!skipWriting) {
			readWriter_.SeekSet(position);
			auto res = readWriter_.Write(rv.begin(), rv.begin() + readBytes);
			if (res && res.value() == readBytes) {
				++statistics_.blocksWritten_;
				statistics_.bytesWritten_ += res.value();
			} else if (res && res.value() == 0) {
				return Error(
					std::error_condition(std::errc::io_error), "Zero write while copying data");
			} else if (res) {
				return Error(
					std::error_condition(std::errc::io_error), "Short write while copying data");
			} else {
				return res.error();
			}
		}
	}
	return NoError;
}
