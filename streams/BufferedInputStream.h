/*
 * BufferedInputStream.h
 *
 *  Created on: Dec 16, 2013
 *      Author: kazu
 */

#ifndef BUFFEREDINPUTSTREAM_H_
#define BUFFEREDINPUTSTREAM_H_

#include <vector>
#include <cstdint>
#include <cstring>
#include "InputStreamPeek.h"
#include "Buffer.h"
#include "StreamException.h"

namespace K {

/**
 * adds additional buffering to an input stream
 */
class BufferedInputStream : public InputStreamPeek {

public:

	/** ctor */
	BufferedInputStream(InputStream* is, const unsigned int blockSize = 4096) :
		is(is), blockSize(blockSize), eof(false) {
		;
	}

	/** dtor */
	virtual ~BufferedInputStream() {
		close();
	}

	int read() override {

		// try to fill the buffer (at least 1 byte)
		fillBuffer(1);

		// buffer empty even if we tried to fill it?
		if (buffer.empty()) {

			// detected EOF? -> we failed
			if (eof) {return ERR_FAILED;}

			// not detected EOF? -> try again later
			return ERR_TRY_AGAIN;

		}

		// everything fine
		return buffer.get();

	}

	int peek() override {

		// try to fill the buffer (at least 1 byte)
		fillBuffer(1);

		// buffer empty even if we tried to fill it?
		if (buffer.empty()) {

			// detected EOF? -> we failed
			if (eof) {return ERR_FAILED;}

			// not detected EOF? -> try again later
			return ERR_TRY_AGAIN;

		}

		// everything fine
		return buffer.peek();

	}

	/** read the given number of bytes into the buffer */
	ssize_t read(uint8_t* data, const size_t len) override {

		// try to fill the buffer (at least len bytes)
		fillBuffer(len);

		// buffer empty even if we tried to fill it?
		if (buffer.empty()) {

			// detected EOF? -> we failed
			if (eof) {return ERR_FAILED;}

			return 0;

		}

		// everything fine
		const size_t toRead = (len < buffer.getNumUsed()) ? (len) : (buffer.getNumUsed());
		memcpy(data, &buffer[0], toRead);
		buffer.remove(toRead);
		return toRead;

	}

	void close() override {
		is->close();
	}

	void skip(const uint64_t n) override {
		(void) n;
		throw StreamException("BufferedInputStream.skip() not yet implemented");
	}

private:

	/** ensure the buffer contains the given number of bytes */
	void fillBuffer(const size_t needed) {

		// buffer already contains enough bytes? -> nothing to do
		if (buffer.getNumUsed() >= needed) {return;}

		// how many bytes to fetch
		const size_t toFetch = (needed < blockSize) ? (blockSize) : (needed);

		// make space for those bytes
		buffer.resize(buffer.getNumUsed() + toFetch);

		// try to fetch from underlying layer
		const ssize_t fetched = is->read(buffer.getFirstFree(), toFetch);
		if (fetched == ERR_FAILED)		{eof = true; return;}
		if (fetched == ERR_TRY_AGAIN)	{return;}

		// everything fine
		buffer.setNumUsed(buffer.getNumUsed() + fetched);

	}

	/** the underlying stream */
	InputStream* is;

	/** the internal buffer */
	Buffer<uint8_t> buffer;

	/** the number of bytes to read every time */
	const unsigned int blockSize;

	/** eof from underlying layer? */
	bool eof;

};

}

#endif /* BUFFEREDINPUTSTREAM_H_ */
