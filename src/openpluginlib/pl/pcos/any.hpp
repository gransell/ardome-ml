// See http://www.boost.org/libs/any for Documentation.

#ifndef OPL_BOOST_ANY_INCLUDED
#define OPL_BOOST_ANY_INCLUDED

// what:  variant type boost::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

// modification of boost::any to support equality operator

#include <algorithm>
#include <cassert>
#include <typeinfo>
#include <list>
#include <vector>

#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>


namespace olib { namespace openpluginlib { namespace pcos {

    template < typename T > T parse_string( const std::wstring& );

    class any
    {
    public: // structors

        any()
          : content(0)
        {
        }

        template<typename ValueType>
        any(const ValueType & value)
          : content(new holder<ValueType>(value))
        {
        }

        any(const any & other)
          : content(other.content ? other.content->clone() : 0)
        {
        }

        ~any()
        {
            delete content;
        }

    public: // modifiers

        any & swap(any & rhs)
        {
            std::swap(content, rhs.content);
            return *this;
        }

        template<typename ValueType>
        any & operator=(const ValueType & rhs)
        {
            any(rhs).swap(*this);
            return *this;
        }

        any & operator=(const any & rhs)
        {
            any(rhs).swap(*this);
            return *this;
        }

	any & from_string( const std::wstring& str )
	{
	    assert( !empty() );
	    content->from_string( str );
	    return *this;
	}

    public: // queries

        bool empty() const
        {
            return !content;
        }

        const std::type_info & type() const
        {
            return content ? content->type() : typeid(void);
        }

        bool operator==(const any & rhs) const
        {
	    return content ? content->operator==( rhs.content ) : content == rhs.content;
        }

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    private: // types
#else
    public: // types (public so any_cast can be non-friend)
#endif

        class placeholder
        {
        public: // structors

            virtual ~placeholder()
            {
            }

        public: // queries

            virtual const std::type_info & type() const = 0;

            virtual placeholder * clone() const = 0;
	    
	    virtual bool operator==( placeholder* ) const = 0;

	    virtual void from_string( const std::wstring& ) = 0;
        };

        template<typename ValueType>
        class holder : public placeholder
        {
        public: // structors

            holder(const ValueType & value)
              : held(value)
            {
            }

        public: // queries

            virtual const std::type_info & type() const
            {
                return typeid(ValueType);
            }

            virtual placeholder * clone() const
            {
                return new holder(held);
            }

	    virtual bool operator==( placeholder* rhs ) const
	    {
		return rhs && type() == rhs->type() && held == static_cast< holder< ValueType >* >( rhs )->held;
	    }

	    virtual void from_string( const std::wstring& str )
	    {
		held = parse_string< ValueType >( str );
	    }

        public: // representation

            ValueType held;

        };

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS

    private: // representation

        template<typename ValueType>
        friend ValueType * any_cast(any *);

        template<typename ValueType>
        friend ValueType * unsafe_any_cast(any *);

#else

    public: // representation (public so any_cast can be non-friend)

#endif

        placeholder * content;

    };

    class bad_any_cast : public std::bad_cast
    {
    public:
        virtual const char * what() const throw()
        {
            return "boost::bad_any_cast: "
                   "failed conversion using boost::any_cast";
        }
    };

    template<typename ValueType>
    ValueType * any_cast(any * operand)
    {
        return operand && operand->type() == typeid(ValueType)
                    ? &static_cast<any::holder<ValueType> *>(operand->content)->held
                    : 0;
    }

    template<typename ValueType>
    const ValueType * any_cast(const any * operand)
    {
        return any_cast<ValueType>(const_cast<any *>(operand));
    }

    template<typename ValueType>
    ValueType any_cast(const any & operand)
    {
        typedef BOOST_DEDUCED_TYPENAME boost::remove_reference<ValueType>::type nonref;

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        // If 'nonref' is still reference type, it means the user has not
        // specialized 'remove_reference'.

        // Please use BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION macro
        // to generate specialization of remove_reference for your class
        // See type traits library documentation for details
        BOOST_STATIC_ASSERT(!is_reference<nonref>::value);
#endif

        const nonref * result = any_cast<nonref>(&operand);
        if(!result)
            boost::throw_exception(bad_any_cast());
        return *result;
    }

    template<typename ValueType>
    ValueType any_cast(any & operand)
    {
        typedef BOOST_DEDUCED_TYPENAME boost::remove_reference<ValueType>::type nonref;

#ifdef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        // The comment in the above version of 'any_cast' explains when this
        // assert is fired and what to do.
        BOOST_STATIC_ASSERT(!is_reference<nonref>::value);
#endif

        nonref * result = any_cast<nonref>(&operand);
        if(!result)
            boost::throw_exception(bad_any_cast());
        return *result;
    }

    // Note: The "unsafe" versions of any_cast are not part of the
    // public interface and may be removed at any time. They are
    // required where we know what type is stored in the any and can't
    // use typeid() comparison, e.g., when our types may travel across
    // different shared libraries.
    template<typename ValueType>
    ValueType * unsafe_any_cast(any * operand)
    {
        return &static_cast<any::holder<ValueType> *>(operand->content)->held;
    }

    template<typename ValueType>
    const ValueType * unsafe_any_cast(const any * operand)
    {
        return any_cast<ValueType>(const_cast<any *>(operand));
    }


typedef std::list< int > int_list;
typedef std::list< unsigned int > uint_list;
typedef std::list< double > double_list;
typedef std::vector<boost::uint8_t> byte_vec;
typedef std::vector<int> int_vec;
typedef std::vector<unsigned int> uint_vec;
typedef std::vector<double> double_vec;
typedef std::vector<boost::int64_t> int64_vec;
typedef std::vector<void*> void_vec;

} } }

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#endif
