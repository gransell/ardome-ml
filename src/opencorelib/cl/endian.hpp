
#ifndef ENDIAN_H_
#define ENDIAN_H_

namespace olib { namespace opencorelib { namespace endian {

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
	return ((t >> 24) & 0xff)
	     | ((t >>  8) & 0xff00)
	     | ((t <<  8) & 0xff0000)
	     | ((t << 24) & 0xff000000);
}
template<> inline uint64_t swap<uint64_t>(const uint64_t& t) {
	return ((t >> 56) & 0xff)
	     | ((t >> 40) & 0xff00)
	     | ((t >> 24) & 0xff0000)
	     | ((t >>  8) & 0xff000000)
	     | ((t <<  8) & 0xff00000000UL)
	     | ((t << 24) & 0xff0000000000UL)
	     | ((t << 40) & 0xff000000000000UL)
	     | ((t << 56) & 0xff00000000000000UL);
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

	inline T get() const {
		return t_;
	}
	inline void set(T t) {
		t_ = t;
	}
private:
	T t_;
};

template<typename T>
struct opposite {
	typedef opposite<T> self;
	opposite() : t_() {}

	inline T get() const {
		return swap(t_);
	}
	inline void set(T t) {
		t_ = swap(t);
	}
private:
	T t_;
};

template<class Super, class Base, typename T>
struct limbo : Base {
	using Base::get;
	using Base::set;

	inline Super& operator=(const T& t) {
		set(t);
		return static_cast<Super&>(*this);
	}
	inline operator T() const {
		return get();
	}
	inline T operator*() const {
		return get();
	}

	template<typename I> Super& operator+=(I i) {
		set(get() + i);
		return static_cast<Super&>(*this);
	}
	template<typename I> Super& operator-=(I i) {
		set(get() - i);
		return static_cast<Super&>(*this);
	}
	template<typename I> Super& operator*=(I i) {
		set(get() * i);
		return static_cast<Super&>(*this);
	}
	template<typename I> Super& operator/=(I i) {
		set(get() / i);
		return static_cast<Super&>(*this);
	}

};

/// A class for storing arbitrary integer types as little endian in memory.
/**	An instance of this class for a certain integer type T, will act as
	that integer type when assigning a value to it. It can be explicitly
	casted into its native integer type T, or implicitly by using the
	*-operator. It can this way be treated just like a variable of type T.
	The difference between a variable of type T and a variable of type
	little<T> is that in memory, little<T> will always be stored in little
	endian regardless of the host endianness.
	An instance will take the same amount of memory as its native integer
	type but must be wrapped around a "pragma pack" statement for this to
	be guaranteed, just like the beginning of this file.
	@author Gustaf R&auml;ntil&auml; */
template<typename T> struct little
#if BYTE_ORDER == LITTLE_ENDIAN
: limbo<little<T>, same<T>, T> {
	typedef same<T> base;
#else
: limbo<little<T>, opposite<T>, T> {
	typedef opposite<T> base;
#endif
	little() {
	}
	little(T t) {
		base::set(t);
	}
};

/// A class for storing arbitrary integer types as big endian in memory.
/**
    This class is identical to little except data is always stored in big
    endian in memory. */
template<typename T> struct big
#if BYTE_ORDER == LITTLE_ENDIAN
: limbo<big<T>, opposite<T>, T> {
	typedef opposite<T> base;
#else
: limbo<big<T>, same<T>, T> {
	typedef same<T> base;
#endif
	big() {
	}
	big(T t) {
		base::set(t);
	}
};

#pragma pack(pop)

} } }

#endif // ENDIAN_H_

