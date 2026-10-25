/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file packet_reader.h Original buffered network save reader, shared by loading and detached replay checks.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 */
#ifndef NETWORK_CORE_PACKET_READER_H
#define NETWORK_CORE_PACKET_READER_H
#include "packet.h"
#include "../../saveload/saveload_filter.h"
#include <algorithm>
#include <deque>
#include <iterator>

/** Read some packets, and when do use that data as initial load filter. */
struct PacketReader : LoadFilter {
	using Buffer = std::deque<uint8_t>; ///< The underlying buffer type that's being use.

	Buffer buffer; ///< Buffer with the raw save game data.
	Buffer::const_iterator iterator; ///< Buffer we're going to write to/read from.
	size_t read_bytes = 0; ///< The total number of read bytes.

	/** Initialise everything. */
	PacketReader() : LoadFilter(nullptr)
	{
	}

	/**
	 * Add a packet to this buffer.
	 * @param p The packet to add.
	 */
	void AddPacket(Packet &p)
	{
		assert(this->read_bytes == 0);
		/** @param source Original received packet bytes. @return Bytes retained for rewindable loading. */
		p.TransferOut([this](std::span<const uint8_t> source) {
			std::ranges::copy(source, std::back_inserter(this->buffer));
			return source.size();
		});
	}

	/** Read only received bytes; no packet request or filesystem dependency.
	 * @param rbuf Caller-owned destination.
	 * @param size Requested maximum byte count.
	 * @return Actual remaining received bytes copied.
	 */
	size_t Read(uint8_t *rbuf, size_t size) override
	{
		/* Limit the amount to read to whatever we still have. */
		size_t read_size = std::min(this->buffer.size() - this->read_bytes, size);

		std::copy_n(this->iterator, read_size, rbuf);
		std::advance(this->iterator, read_size);
		this->read_bytes += read_size;

		return read_size;
	}

	/** Rewind the received map buffer before reconstructing every codec and read buffer. */
	void Reset() override
	{
		this->read_bytes = 0;
		this->iterator = this->buffer.cbegin();
	}
};

#endif /* NETWORK_CORE_PACKET_READER_H */
