#pragma once

namespace Olib { namespace Opencorelib {

public value class RationalTime
{
public:
	RationalTime(__int64 num, __int64 den)
	{
		_num = num;
		_den = den;
	}

	property __int64 Num
	{
		__int64 get() {return _num;}
		void set(__int64 val) {_num = val;}
	}

	property __int64 Den
	{
		__int64 get() {return _den;}
		void set(__int64 val) {_den = val;}
	}

private:
	__int64 _num;
	__int64 _den;
};

} }
