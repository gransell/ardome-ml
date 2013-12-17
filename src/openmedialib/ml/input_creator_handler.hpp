
// Copyright (C) 2009 Vizrt
// Released under the LGPL.

#ifndef OPENMEDIALIB_ID_TO_INPUT_HANDLER_H_
#define OPENMEDIALIB_ID_TO_INPUT_HANDLER_H_

#define LOKI_CLASS_LEVEL_THREADING

#include <loki/Singleton.h>
#include <openmedialib/ml/input.hpp>
#include <map>
#include <boost/function.hpp>
#include <boost/thread/recursive_mutex.hpp>


namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC input_creator : public boost::noncopyable
{
public:
	
	typedef boost::function< input_type_ptr() > creator_function;

	input_creator( const olib::t_string& identifier, creator_function creator )
		: id_( identifier )
		, creator_( creator )
	{}
	virtual ~input_creator( )
	{}

	const olib::t_string& identifier( ) const { return id_; }

	input_type_ptr create_input( ) const
	{
		return creator_( );
	}

private:
	/// The id for this creator. This should be a string that uniquely defines the input that this creator
	/// will create. For example a URL to a file.
	olib::t_string id_;
	creator_function creator_;
};

typedef boost::shared_ptr< input_creator > input_creator_ptr;

class ML_DECLSPEC input_creator_handler : public boost::noncopyable
{
public:
	virtual ~input_creator_handler( )
	{
	}

	input_creator_ptr input_creator_for_id( const olib::t_string &identifier )
	{
		input_creator_ptr ret;

		std::map< olib::t_string, input_creator_ptr >::const_iterator found;
		{
			boost::recursive_mutex::scoped_lock lck( mtx_ );
			if( ( found = creators_.find( identifier ) ) != creators_.end( ) )
				ret = found->second;
		}

		return ret;;
	}

	void register_creator( const input_creator_ptr& ctor )
	{
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		creators_[ ctor->identifier( ) ] = ctor;
	}

	void unregister_creator( const olib::t_string& identifier )
	{
		boost::recursive_mutex::scoped_lock lck( mtx_ );
		creators_.erase( identifier );
	}

	template <typename T> friend struct Loki::CreateUsingNew;

private:
	input_creator_handler( )
	{
	}

	boost::recursive_mutex mtx_;
	std::map< olib::t_string, input_creator_ptr > creators_;
};

class ML_DECLSPEC the_input_creator_handler
{
public:
	static input_creator_handler &instance( );
};

} /* ml */
} /* openmedialib */
} /* olib */

#endif // #ifndef OPENMEDIALIB_ID_TO_INPUT_HANDLER_H_
