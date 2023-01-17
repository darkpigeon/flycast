/*
	Copyright 2021 flyinghead

	This file is part of Flycast.

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Flycast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Flycast.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once
#include "types.h"

#include <limits>

class SerializeBase
{
public:
	enum Version : int32_t {
		V1,
		V2,
		V3,
		V4,
		V5_LIBRETRO,
		V6_LIBRETRO,
		V7_LIBRETRO,
		V8_LIBRETRO,
		V9_LIBRETRO,
		V10_LIBRETRO,
		V11_LIBRETRO,
		V12_LIBRETRO,
		V13_LIBRETRO,
		VLAST_LIBRETRO = V13_LIBRETRO,

		V5 = 800,
		V6,
		V7,
		V8,
		V9,
		V10,
		V11,
		V12,
		V13,
		V14,
		V15,
		V16,
		V17,
		V18,
		V19,
		V20,
		V21,
		V22,
		V23,
		V24,
		V25,
		V26,
		V27,
		V28,
		V29,
		V30,
		V31,
		Current = V31,

		Next = Current + 1,
	};

	size_t size() const { return _size; }
	bool rollback() const { return _rollback; }

protected:
	SerializeBase(size_t limit, bool rollback)
		: _size(0), limit(limit), _rollback(rollback) {}

	size_t _size;
	size_t limit;
	bool _rollback;
};

class Deserializer : public SerializeBase
{
public:
	class Exception : public std::runtime_error
	{
	public:
		Exception(const char *msg) : std::runtime_error(msg) {}
	};

	Deserializer(const void *data, size_t limit, bool rollback = false)
		: SerializeBase(limit, rollback), data((const u8 *)data)
	{
		deserialize(_version);
		if (_version > V13_LIBRETRO && _version < V5)
			throw Exception("Unsupported version");
		if (_version > Current)
			throw Exception("Version too recent");
	}

	template<typename T>
	void deserialize(T& obj)
	{
		doDeserialize(&obj, sizeof(T));
	}
	template<typename T>
	void deserialize(T *obj, size_t count)
	{
		doDeserialize(obj, sizeof(T) * count);
	}

	template<typename T>
	void skip(Version minVersion = Next)
	{
		skip(sizeof(T), minVersion);
	}
	void skip(size_t size, Version minVersion = Next)
	{
		if (_version >= minVersion)
			return;
		if (this->_size + size > limit)
		{
			WARN_LOG(SAVESTATE, "Savestate overflow: current %d limit %d sz %d", (int)this->_size, (int)limit, (int)size);
			throw Exception("Invalid savestate");
		}
		data += size;
		this->_size += size;
	}

	Version version() const { return _version; }

private:
	void doDeserialize(void *dest, size_t size)
	{
		if (this->_size + size > limit)	// FIXME one more test vs. current
		{
			WARN_LOG(SAVESTATE, "Savestate overflow: current %d limit %d sz %d", (int)this->_size, (int)limit, (int)size);
			throw Exception("Invalid savestate");
		}
		memcpy(dest, data, size);
		data += size;
		this->_size += size;
	}

	Version _version;
	const u8 *data;
};

class Serializer : public SerializeBase
{
public:
	Serializer()
		: Serializer(nullptr, std::numeric_limits<size_t>::max(), false) {}

	Serializer(void *data, size_t limit, bool rollback = false)
		: SerializeBase(limit, rollback), data((u8 *)data)
	{
		Version v = Current;
		serialize(v);
	}

	template<typename T>
	void serialize(const T& obj)
	{
		doSerialize(&obj, sizeof(T));
	}
	template<typename T>
	void serialize(const T *obj, size_t count)
	{
		doSerialize(obj, sizeof(T) * count);
	}

	template<typename T>
	void skip()
	{
		skip(sizeof(T));
	}
	void skip(size_t size)
	{
		if (data != nullptr)
			data += size;
		this->_size += size;
	}
	bool dryrun() const { return data == nullptr; }

private:
	void doSerialize(const void *src, size_t size)
	{
		if (data != nullptr)
		{
			memcpy(data, src, size);
			data += size;
		}
		this->_size += size;
	}

	u8 *data;
};

template<typename T>
Serializer& operator<<(Serializer& ctx, const T& obj) {
	ctx.serialize(obj);
	return ctx;
}

template<typename T>
Deserializer& operator>>(Deserializer& ctx, T& obj) {
	ctx.deserialize(obj);
	return ctx;
}

void dc_serialize(Serializer& ctx);
void dc_deserialize(Deserializer& ctx);
