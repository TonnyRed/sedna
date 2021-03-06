/*
 * File:  lip_class.h
 * Copyright (C) 2006 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
 */

#ifndef __LIP_CLASS_H
#define __LIP_CLASS_H

#include "common/sedna.h"
#include "tr/executor/base/lip/lip.h"

// LIP - Long Integer Package
// it's a wrapper for lip C library (arbitrary long integers)
class lip {
protected:
    verylong x;

public:
// Constructors/Destructor
	lip() 
    { 
        x = 0; 
    }
	lip(const lip& Value)
    {
        x = 0;
        zcopy(Value.x, &x);
    }
	lip(int32_t Value)
    {
        x = 0;
        zintoz(Value, &x);
    }
	lip(uint32_t Value)
    {
        x = 0;
        zuintoz(Value, &x);
    }
	lip(int64_t Value)
    {
        x = 0;
        uint64_t vv = 0;
        bool s = false;
        if (Value < 0)
        {
            vv = (uint64_t)(-Value);
            s = true;
        }
        else
        {
            vv = (uint64_t)Value;
            s = false;
        }        
        verylong upper = 0, lower = 0;
        zuintoz((uint32_t)(vv >> 32), &upper);
        zuintoz((uint32_t)(vv << 32 >> 32), &lower);
        zlshift(upper, 32, &upper);
        zor(upper, lower, &x);
        zfree(&upper);
        zfree(&lower);
        if (s) znegate(&x);
    }
    lip(const char *str)
    {
        x = 0;
        zsread((char*)str, &x);
    }

	virtual ~lip()
    {
        zfree(&x);
    }

// Conversion Operators
	operator int32_t() const 
    {
        return (int32_t)ztoint(x);
    }
	operator uint32_t() const 
    {
        return (uint32_t)ztouint(x);
    }

// Unary Operators
	lip& operator ++()		// Prefix increment
    {
        zsadd(x, 1, &x);
        return *this;
    }

	lip operator ++(int)	// Postfix increment
    {
        lip res(*this);
        zsadd(x, 1, &x);
        return res;
    }

	lip& operator --()		// Prefix decrement
    {
        zsub(x, lip((int32_t)1).x, &x);
        return *this;
    }

	lip operator --(int)	// Postfix decrement
    {
        lip res(*this);
        zsub(x, lip((int32_t)1).x, &x);
        return res;
    }

	lip operator -() const	// Negation
    {
        lip res(*this);
        znegate(&(res.x));
        return res;
    }

// Binary Operators
	void Div(const lip &Divisor, lip &Quotient, lip &Remainder) const 
    {
        zdiv(x, Divisor.x, &(Quotient.x), &(Remainder.x));
	}

	lip operator +(const lip& Value) const
    {
        lip res;
        zadd(x, Value.x, &(res.x));
        return res;
    }

	lip operator +(int32_t Value) const
    {
        lip res;
        zsadd(x, Value, &(res.x));
        return res;
    }

	lip operator -(const lip& Value) const
    {
        lip res;
        zsub(x, Value.x, &(res.x));
        return res;
    }

	lip operator -(int32_t Value) const
    {
        lip res;
        zsub(x, lip(Value).x, &(res.x));
        return res;
    }

	lip operator *(const lip& Value) const
    {
        lip res;
        zmul(x, Value.x, &(res.x));
        return res;
    }

	lip operator *(int32_t Value) const
    {
        lip res;
        zsmul(x, Value, &(res.x));
        return res;
    }
	
	lip operator /(const lip& Value) const
    {
        lip q, r;
        zdiv(x, Value.x, &(q.x), &(r.x));
        return q;
    }

	lip operator /(int32_t Value) const
    {
        lip q;
        zsdiv(x, Value, &(q.x));
        return q;
    }

	lip operator %(const lip& Value) const
    {
        lip q, r;
        zdiv(x, Value.x, &(q.x), &(r.x));
        return r;
    }

	lip operator %(int32_t Value) const
    {
        lip q;
        long r = zsdiv(x, Value, &(q.x));
        return lip((int32_t)r);
    }

	lip operator &(const lip& Value) const
    {
        lip res;
        zand(x, Value.x, &(res.x));
        return res;
    }

	lip operator |(const lip& Value) const
    {
        lip res;
        zor(x, Value.x, &(res.x));
        return res;
    }

	lip operator ^(const lip& Value) const
    {
        lip res;
        zxor(x, Value.x, &(res.x));
        return res;
    }

	lip operator <<(int32_t nBits) const
    {
        lip res;
        zlshift(x, nBits, &(res.x));
        return res;
    }

	lip operator >>(int32_t nBits) const
    {
        lip res;
        zrshift(x, nBits, &(res.x));
        return res;
    }


// Logical Operators
	bool operator !=(const lip& Value) const
    {
        return zcompare(x, Value.x) != 0;
    }

	bool operator ==(const lip& Value) const
    {
        return zcompare(x, Value.x) == 0;
    }

	bool operator <(const lip& Value) const
    {
        return zcompare(x, Value.x) < 0;
    }

	bool operator <=(const lip& Value) const
    {
        return zcompare(x, Value.x) <= 0;
    }

	bool operator >(const lip& Value) const
    {
        return zcompare(x, Value.x) > 0;
    }

	bool operator >=(const lip& Value) const
    {
        return zcompare(x, Value.x) >= 0;
    }

// Assignment Operators

	lip& operator =(const lip& Value)
    {
        zcopy(Value.x, &x);
        return *this;
    }

	lip& operator +=(const lip& Value) { return *this = operator +(Value); }
	lip& operator +=(int32_t    Value) { return *this = operator +(Value); }
	lip& operator -=(const lip& Value) { return *this = operator -(Value); }
	lip& operator -=(int32_t    Value) { return *this = operator -(Value); }
	lip& operator *=(const lip& Value) { return *this = operator *(Value); }
	lip& operator *=(int32_t    Value) { return *this = operator *(Value); }
	lip& operator /=(const lip& Value) { return *this = operator /(Value); }
	lip& operator /=(int32_t    Value) { return *this = operator /(Value); }
	lip& operator %=(const lip& Value) { return *this = operator %(Value); }
	lip& operator %=(int32_t    Value) { return *this = operator %(Value); }
	lip& operator &=(const lip& Value) { return *this = operator &(Value); }
	lip& operator |=(const lip& Value) { return *this = operator |(Value); }
	lip& operator ^=(const lip& Value) { return *this = operator ^(Value); }
	lip& operator <<=(int32_t nBits)
    {
        zlshift(x, nBits, &x);
        return *this;
    }
	lip& operator >>=(int32_t nBits)
    {
        zrshift(x, nBits, &x);
        return *this;
    }

// Output functions and operators
    long format(char *str)
    {
        return zswrite(str, x);
    }

    char* format(char *str, int *length)
    {
        if (length)
            *length = zswrite(str, x);
        else 
            zswrite(str, x);

        return str;
    }

};


#endif
