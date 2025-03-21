// ******  rpcmple for c++ v0.2  ******
// Copyright (C) 2024 Carlo Seghi. All rights reserved.
// Author Carlo Seghi github.com/acs48.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the MIT license
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Library General Public License for more details.
//
// Use of this source code is governed by the MIT license
// License that can be found in the LICENSE file.

#ifndef RPCMPLE_H
#define RPCMPLE_H

#include <cstdint>
#include <cstring>

#include "spdlog/spdlog.h"

#include <codecvt>

namespace rpcmple
{
	// Function to convert std::wstring to UTF-8 encoded std::string
	std::string wstring_to_utf8(const std::wstring& wstr)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(wstr);
	}

	inline bool isMachineLittleEndian()
	{
		int num = 1;
		return *(char*)&num == 1;
	}

	inline uint64_t swapEndian64(uint64_t val)
	{
		val = (val & 0x00000000FFFFFFFF) << 32 | (val & 0xFFFFFFFF00000000) >> 32;
		val = (val & 0x0000FFFF0000FFFF) << 16 | (val & 0xFFFF0000FFFF0000) >> 16;
		val = (val & 0x00FF00FF00FF00FF) << 8 | (val & 0xFF00FF00FF00FF00) >> 8;
		return val;
	}

	inline uint32_t swapEndian32(uint32_t val)
	{
		val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
		return (val << 16) | (val >> 16);
	}

	inline uint16_t swapEndian16(uint16_t val)
	{
		return (val << 8) | (val >> 8);
	}

	inline void doubleToBytes(double dbl, uint8_t bytes[8], bool isLittleEndian)
	{
		uint64_t val;
		std::memcpy(&val, &dbl, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian64(val);
		}

		std::memcpy(bytes, &val, sizeof val);
	}

	inline void int64ToBytes(int64_t longint, uint8_t bytes[8], bool isLittleEndian)
	{
		uint64_t val;
		std::memcpy(&val, &longint, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian64(val);
		}

		std::memcpy(bytes, &val, sizeof val);
	}

	inline void uint64ToBytes(uint64_t dbl, uint8_t bytes[8], bool isLittleEndian)
	{
		uint64_t val;
		std::memcpy(&val, &dbl, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian64(val);
		}

		std::memcpy(bytes, &val, sizeof val);
	}

	inline void uint32ToBytes(uint32_t dbl, uint8_t bytes[4], bool isLittleEndian)
	{
		uint32_t val;
		std::memcpy(&val, &dbl, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian32(val);
		}

		std::memcpy(bytes, &val, sizeof val);
	}

	inline void uint16ToBytes(uint16_t dbl, uint8_t bytes[2], bool isLittleEndian)
	{
		uint16_t val;
		std::memcpy(&val, &dbl, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian16(val);
		}

		std::memcpy(bytes, &val, sizeof val);
	}

	inline double bytesToDouble(const uint8_t bytes[8], bool isLittleEndian)
	{
		uint64_t val;
		std::memcpy(&val, bytes, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian64(val);
		}

		double dbl;
		std::memcpy(&dbl, &val, sizeof dbl);
		return dbl;
	}

	inline uint16_t bytesToUint16(const uint8_t bytes[2], bool isLittleEndian)
	{
		uint16_t val;
		std::memcpy(&val, bytes, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian16(val);
		}

		return val;
	}

	inline uint32_t bytesToUint32(const uint8_t bytes[4], bool isLittleEndian)
	{
		uint32_t val;
		std::memcpy(&val, bytes, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian32(val);
		}

		return val;
	}

	inline uint64_t bytesToUint64(const uint8_t bytes[8], bool isLittleEndian)
	{
		uint64_t val;
		std::memcpy(&val, bytes, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian64(val);
		}

		return val;
	}

	inline int64_t bytesToInt64(const uint8_t bytes[8], bool isLittleEndian)
	{
		uint64_t val;
		std::memcpy(&val, bytes, sizeof val);

		if (isLittleEndian != isMachineLittleEndian())
		{
			val = swapEndian64(val);
		}

		int64_t dbl;
		std::memcpy(&dbl, &val, sizeof dbl);
		return dbl;
	}
}

#endif //RPCMPLE_H
