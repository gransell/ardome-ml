
#ifndef ENDIAN_H_
#define ENDIAN_H_

namespace aml { namespace endian {

// This will basically enter a context where struct member alignment will be
// one byte, i.e. per-byte alignment. In other words, structs will not be
// "packed". This must be done surrounding all usage of these helper structs!
// It is reset at the end of this file.
#pragma pack(push, 1)

template<typename T> inline T swap(const T& t);

template<> inline uint8_t swap<uint8_t>(const uint8_t& t) {
	return t;
}
template<> inline uint16_t swap<uint16_t>(const uint16_t& t) {
	return ((t & 0xff) << 8) + ((t >> 8) & 0xff);
}
template<> inline uint32_t swap<uint32_t>(const uint32_t& t) {
	return (t >> 24) & 0xff
	     | (t >>  8) & 0xff00
	     | (t <<  8) & 0xff0000
	     | (t << 24) & 0xff000000;
}
template<> inline uint64_t swap<uint64_t>(const uint64_t& t) {
	return (t >> 56) & 0xff
	     | (t >> 40) & 0xff00
	     | (t >> 24) & 0xff0000
	     | (t >>  8) & 0xff000000
	     | (t <<  8) & 0xff00000000UL
	     | (t << 24) & 0xff0000000000UL
	     | (t << 40) & 0xff000000000000UL
	     | (t << 56) & 0xff00000000000000UL;
}
template<> inline int8_t swap<int8_t>(const int8_t& t) {
	return t;
}
template<> inline int16_t swap<int16_t>(const int16_t& t) {
	return (int16_t)swap((uint16_t)t);
}
template<> inline int32_t swap<int32_t>(const int32_t& t) {
	return (int32_t)swap((uint32_t)t);
}
template<> inline int64_t swap<int64_t>(const int64_t& t) {
	return (int64_t)swap((uint64_t)t);
}


template<typename T>
struct same {
	typedef same<T> self;
	same() : t_() {}

	inline self& operator=(const T& t) {
		t_ = t;
		return *this;
	}
	inline operator T() const {
		return t_;
	}
	inline T operator*() const {
		return t_;
	}
private:
	T t_;
};

template<typename T>
struct opposite {
	typedef opposite<T> self;
	opposite() : t_() {}

	inline self& operator=(const T& t) {
		t_ = swap(t);
		return *this;
	}
	inline operator T() const {
		return swap(t_);
	}
	inline T operator*() const {
		return swap(t_);
	}
private:
	T t_;
};

template<typename T> struct little
#if BYTE_ORDER == LITTLE_ENDIAN
: same<T> {
	typedef same<T> base;
#else
: opposite<T> {
	typedef opposite<T> base;
#endif
	little() {
	}
	little(T t) {
		this->base::operator=(t);
	}
	inline little<T>& operator=(const T& t) {
		this->base::operator=(t);
		return *this;
	}
};
template<typename T> struct big
#if BYTE_ORDER == LITTLE_ENDIAN
: opposite<T> {
	typedef opposite<T> base;
#else
: same<T> {
	typedef same<T> base;
#endif
	big() {
	}
	big(T t) {
		this->base::operator=(t);
	}
	inline big<T>& operator=(const T& t) {
		this->base::operator=(t);
		return *this;
	}
};

#pragma pack(pop)

} }

#endif // ENDIAN_H_

