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

#ifndef FILEIO_HPP
#define FILEIO_HPP
#include "common/io.hpp"
#include "platformfs.h"

namespace mender {
namespace io {

class FileWriter : public common::io::Writer {
public:
	FileWriter(File f);
	virtual ExpectedSize Write(
		vector<uint8_t>::const_iterator start, vector<uint8_t>::const_iterator end) override;

	File GetFile() const {
		return mFd;
	}

protected:
	File mFd;
};

class LimitedFlushingWriter : public FileWriter {
public:
	LimitedFlushingWriter(File f, size_t limit, uint32_t flushInterval = 1);
	virtual ExpectedSize Write(
		vector<uint8_t>::const_iterator start, vector<uint8_t>::const_iterator end) override;

protected:
	size_t mWritingLimit {0};
	uint32_t mFlushIntervalBytes;
	uint32_t mUnflushedBytesWritten {0};
};

class FileReader : public common::io::Reader {
public:
	FileReader(File fd);
	virtual ExpectedSize Read(
		vector<uint8_t>::iterator start, vector<uint8_t>::iterator end) override;
	virtual ExpectedSize Tell() const;

	File GetFile() const {
		return mFd;
	}

protected:
	File mFd;
};

class InputStreamReader : public FileReader {
public:
	InputStreamReader();
	virtual ExpectedSize Read(
		vector<uint8_t>::iterator start, vector<uint8_t>::iterator end) override;
	virtual ExpectedSize Tell() const override;

protected:
	size_t mReadBytes;
};

class FileReadWriter : public common::io::ReadWriter {
public:
	FileReadWriter(File f);
	virtual ExpectedSize Read(
		vector<uint8_t>::iterator start, vector<uint8_t>::iterator end) override;
	virtual ExpectedSize Write(
		vector<uint8_t>::const_iterator start, vector<uint8_t>::const_iterator end) override;

	File GetFile() const {
		return mFd;
	}

protected:
	File mFd;
};

class FileReadWriterSeeker : public FileReadWriter {
public:
	FileReadWriterSeeker(FileWriter &writer);

	virtual ExpectedSize Write(
		vector<uint8_t>::const_iterator start, vector<uint8_t>::const_iterator end) override;
	Error SeekSet(uint64_t pos);
	virtual ExpectedSize Tell() const;

protected:
	FileWriter &mWriter;
};

} // namespace io
} // namespace mender
#endif // FILEIO_H
