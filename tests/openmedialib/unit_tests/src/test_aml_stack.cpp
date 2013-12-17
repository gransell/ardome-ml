#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/ml.hpp>
#include <openmedialib/ml/stack.hpp>

using namespace olib::openmedialib::ml;

BOOST_AUTO_TEST_SUITE( aml_stack )

BOOST_AUTO_TEST_CASE( empty_pop )
{
	// This will result in an underflow

	BOOST_REQUIRE_THROW( stack( ).pop( ), std::exception );
}

BOOST_AUTO_TEST_CASE( null_push )
{
	// Pushing a null pointer to the stack will cause an exception

	BOOST_REQUIRE_THROW( stack( ).push( input_type_ptr( ) ), std::exception );
}

BOOST_AUTO_TEST_CASE( gigo )
{
	// Most things which are pushed to stack are interpreted as inputs, thus when giving
	// it garbage which doesn't match anything the stack knows about, it becomes the same 
	// as trying to pass a null pointer.

	BOOST_REQUIRE_THROW( stack( ).push( L"gigo" ), std::exception );
}

BOOST_AUTO_TEST_CASE( test_underflow )
{
	// This will result in an underflow

	stack s;
	BOOST_REQUIRE_NO_THROW( s.push( L"test:" ) );
	BOOST_REQUIRE( s.pop( ) );
	BOOST_REQUIRE_THROW( s.pop( ), std::exception );
}

BOOST_AUTO_TEST_CASE( test_null_pop )
{
	// An attempt to push a null pointer doesn't affect the rest of the stack

	stack s;
	BOOST_REQUIRE_NO_THROW( s.push( L"test:" ) );
	BOOST_REQUIRE_THROW( s.push( input_type_ptr( ) ), std::exception );
	input_type_ptr test = s.pop( );
	BOOST_REQUIRE( test );
	BOOST_REQUIRE( test->get_uri( ) == L"test:" );
}

BOOST_AUTO_TEST_CASE( property )
{
	// Confirm that property assignments work

	input_type_ptr i = stack( ).push( L"colour: r=255 g=128 b=255" ).pop( );
	BOOST_REQUIRE( i );
	BOOST_CHECK( i->property( "r" ).value< int >( ) == 255 );
	BOOST_CHECK( i->property( "g" ).value< int >( ) == 128 );
	BOOST_CHECK( i->property( "b" ).value< int >( ) == 255 );
}

BOOST_AUTO_TEST_CASE( property_null )
{
	// Confirm that property assignments to an empty stack fail

	BOOST_CHECK_THROW( stack( ).push( "gremlin=mogwai" ), std::exception );
}

BOOST_AUTO_TEST_CASE( property_fail )
{
	// Confirm that invalid property assignments fail - it will leave the targeted input on the 
	// stack. Both the name of the property must exist and the value given must be convertible to the
	// type of the property, thus each of the following will fail, but the colour: will remain on
	// the stack.

	stack s;
	BOOST_REQUIRE_NO_THROW( s.push( L"colour:" ) );
	BOOST_CHECK_THROW( s.push( "gremlin=mogwai" ), std::exception );
	BOOST_CHECK_THROW( s.push( "r=invalid-type" ), std::exception );
	BOOST_CHECK_THROW( s.push( "r=" ), std::exception );
	BOOST_REQUIRE_NO_THROW( s.pop( ) );
}

BOOST_AUTO_TEST_CASE( gigo_sort_of )
{
	// When things do match but definitely don't exist, things get a little more complicated :).
	//
	// The push will succeed and the corresponding input will be pushed to the stack, subsequent
	// assignments to that inputs properties will be valid or will fail if they're stoopid, and the 
	// eventual failure resulting from the garbage will occur on a subsequent pop or on anything else 
	// which enforces the input's init.

	stack s;
	BOOST_REQUIRE_NO_THROW( s.push( L"gigo.avi" ) );
	BOOST_CHECK_NO_THROW( s.push( L"audio_index=-1" ) );
	BOOST_CHECK_THROW( s.push( "gremlin=mogwai" ), std::exception );
	BOOST_CHECK_THROW( s.push( "r=invalid-type" ), std::exception );
	BOOST_CHECK_THROW( s.push( "r=" ), std::exception );
	BOOST_REQUIRE_THROW( s.pop( ), std::exception );

	// ... this is an important design issue here - typically, input properties are ignored after
	// initialisation so it should not be assumed they can be changed (there are exceptions, but as a
	// general rule, it always safer to replace inputs rather than attempt to change them via 
	// properties). Filter properties can be mutable, but care should be taken with these too - it's
	// always implementation specific.
}

BOOST_AUTO_TEST_CASE( property_push )
{
	// Confirm that property assignments work via separate pushes with assorted whitespace

	input_type_ptr i = stack( ).push( L"  colour:" ).push( L"   r=255  " ).push( L" g=128 " ).push( L" b=255" ).pop( );
	BOOST_REQUIRE( i );
	BOOST_CHECK( i->property( "r" ).value< int >( ) == 255 );
	BOOST_CHECK( i->property( "g" ).value< int >( ) == 128 );
	BOOST_CHECK( i->property( "b" ).value< int >( ) == 255 );
}

BOOST_AUTO_TEST_CASE( values )
{
	// int, float and strings can be pushed to the stack in a manner which won't be intepreted as inputs

	stack s;
	input_type_ptr i;

	// Numbers
	BOOST_REQUIRE_NO_THROW( s.push( L"255" ) );
	BOOST_REQUIRE_NO_THROW( i = s.pop( ) );
	BOOST_REQUIRE( i );
	BOOST_CHECK( i->get_uri( ) == L"255" );

	// Negative Numbers
	BOOST_REQUIRE_NO_THROW( s.push( L"-255" ) );
	BOOST_REQUIRE_NO_THROW( i = s.pop( ) );
	BOOST_REQUIRE( i );
	BOOST_CHECK( i->get_uri( ) == L"-255" );

	// Positive Numbers
	BOOST_REQUIRE_NO_THROW( s.push( L"+255" ) );
	BOOST_REQUIRE_NO_THROW( i = s.pop( ) );
	BOOST_REQUIRE( i );
	BOOST_CHECK( i->get_uri( ) == L"255" );

	// String
	BOOST_REQUIRE_NO_THROW( s.push( L"$ gigo" ) );
	BOOST_REQUIRE_NO_THROW( i = s.pop( ) );
	BOOST_REQUIRE( i );
	BOOST_CHECK( i->get_uri( ) == L"gigo" );

	// String with spaces
	BOOST_REQUIRE_NO_THROW( s.push( L"$ \"gigo is gogi\"" ) );
	BOOST_REQUIRE_NO_THROW( i = s.pop( ) );
	BOOST_REQUIRE( i );
	BOOST_CHECK( i->get_uri( ) == L"gigo is gogi" );

	// Oddball string usage - don't think it's right - should probably throw on the $ 
	BOOST_CHECK( s.push( L"$" ).push( "hello" ).pop( )->get_uri( ) == L"hello" );
}

BOOST_AUTO_TEST_CASE( property_stack )
{
	// Properties can be assigned from values which have been pushed on to the stack. Note that during
	// assignment of each property, the top of stack (colour:) is removed - when %s is found in the value
	// it is replaced by the current top of stack, and the resultant colour: is returned to the stack.
	// Thus assignment for properties is left to right (r, g, b), but the values are taken right to left.
	// iyswim.

	input_type_ptr i = stack( ).push( L"1 2 3 colour: r=%s g=%s b=%s" ).pop( );
	BOOST_REQUIRE( i );
	BOOST_CHECK( i->property( "r" ).value< int >( ) == 3 );
	BOOST_CHECK( i->property( "g" ).value< int >( ) == 2 );
	BOOST_CHECK( i->property( "b" ).value< int >( ) == 1 );
}

BOOST_AUTO_TEST_CASE( length )
{
	// Determine how many frames are in the current input at the top of the stack using length?

	stack s;
	BOOST_CHECK_THROW( s.push( "length?" ), std::exception );
	BOOST_CHECK_NO_THROW( s.push( "test:" ) );
	BOOST_CHECK_NO_THROW( s.push( "length?" ) );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"250" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"test:" );
	BOOST_CHECK_THROW( s.push( "length?" ), std::exception );
}

BOOST_AUTO_TEST_CASE( depth )
{
	// depth? deposits an input with a uri of the number of inputs currently on the stack

	stack s;
	input_type_ptr i;

	BOOST_REQUIRE( i = s.push( L"depth?" ).pop( ) );
	BOOST_CHECK( i->get_uri( ) == L"0" );

	BOOST_REQUIRE( i = s.push( L"test: depth?" ).pop( ) );
	BOOST_CHECK( i->get_uri( ) == L"1" );

	BOOST_REQUIRE( i = s.pop( ) );
	BOOST_CHECK( i->get_uri( ) == L"test:" );

	BOOST_REQUIRE( i = s.push( L"depth?" ).pop( ) );
	BOOST_CHECK( i->get_uri( ) == L"0" );
}

BOOST_AUTO_TEST_CASE( filter )
{
	// When a filter is pushed to the stack, the stack effectively shrinks by the number of slots the filter has, 
	// so in this case the depth? after the push of the filter remains as 1.

	stack s;
	input_type_ptr i;

	s.push( "test:" );
	BOOST_REQUIRE( i = s.push( L"depth?" ).pop( ) );
	BOOST_CHECK( i->get_uri( ) == L"1" );

	s.push( "filter:invert" );
	BOOST_REQUIRE( i = s.push( L"depth?" ).pop( ) );
	BOOST_CHECK( i->get_uri( ) == L"1" );

}

BOOST_AUTO_TEST_CASE( filter_underflow )
{
	// When a filter is pushed on to a stack which does not have the number of entries it requires, an error will occur,
	// but it won't happen where you might expect it... it's the following operation which causes a pop which fails.
	// Also note that any (valid or otherwise) commands pushed in the same call will not be executed.

	stack s;
	input_type_ptr i;

	BOOST_CHECK_NO_THROW( s.push( "filter:invert" ) );
	BOOST_CHECK_THROW( s.push( L"depth?" ), std::exception );
	BOOST_CHECK_THROW( s.pop( ), std::exception );

	BOOST_CHECK_NO_THROW( s.push( "filter:invert" ) );
	BOOST_CHECK_THROW( s.push( L"filter:invert test:" ), std::exception );
	BOOST_CHECK_THROW( s.pop( ), std::exception );

	BOOST_CHECK_NO_THROW( s.push( "filter:chroma" ) );
	BOOST_CHECK_NO_THROW( s.push( "u=16" ) );
	BOOST_CHECK_THROW( s.push( L"depth? this-is-rubbish-which-will-not-be-executed" ), std::exception );
	BOOST_CHECK_THROW( s.pop( ), std::exception );

	// .. the reason for this is because property assignments can change the number of the slots a filter connects to, 
	// thus filter connection is always deferred until the following operation which forces a pop. See gigo_sort_of 
	// above - both are handled in the same way.
	//
	// This also means that if an error occurs during the connection (normally due to an underflow), the resultant 
	// connected filter and all of it's consumed stack items will disappear from the stack.

	BOOST_REQUIRE_NO_THROW( s.push( "test: test: test: test: test: test: test: test:" ) );
	BOOST_REQUIRE_NO_THROW( s.push( "filter:playlist slots=9" ) );
	BOOST_CHECK_THROW( s.push( L"depth?" ), std::exception );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"0" );
}

BOOST_AUTO_TEST_CASE( playlist )
{
	// As a follow up to the depth test, you can create a simple playlist of any number of items on the stack, using:

	stack s;
	input_type_ptr i;

	s.push( "test: test: test: test: test: test: test: test:" );
	s.push( "depth? filter:playlist slots=%s" );

	BOOST_REQUIRE( i = s.pop( ) );
	BOOST_CHECK( i->property( "slots" ).value< int >( ) == 8 );
}

BOOST_AUTO_TEST_CASE( acquisition )
{
	// When an input is pushed to the stack which is 'incomplete' (ie: the furtherest downstream filter has null slots),
	// a pop will attemtpt to populate those slots from the stack contents.

	stack s;
	input_type_ptr i;

	s.push( "test:" );
	s.push( create_filter( L"invert" ) );
	BOOST_REQUIRE( i = s.pop( ) );
	BOOST_REQUIRE( i->fetch_slot( ) );
	BOOST_CHECK( i->fetch_slot( )->get_uri( ) == L"test:" );

	s.push( "colour: out=25 r=255" );
	s.push( "colour: out=25 g=255" );
	s.push( create_filter( L"playlist" ) );
	BOOST_REQUIRE( i = s.pop( ) );
	BOOST_REQUIRE( i->fetch_slot( 0 ) );
	BOOST_REQUIRE( i->fetch_slot( 1 ) );
	BOOST_CHECK( i->fetch_slot( 0 )->property( "r" ).value< int >( ) == 255 );
	BOOST_CHECK( i->fetch_slot( 1 )->property( "g" ).value< int >( ) == 255 );

	// NB: This does not currently apply to upstream filters - doing so would require a full walk of the graph each time 
	// a pop occurs and would require some decision regarding the order in which things are done. Probably better to add
	// an explcit function to carry out this operation.
}

BOOST_AUTO_TEST_CASE( decap )
{
	// decap (being an abbreviation of decapitate) removes the last filter if it exists and returns all of its
	// connected slots to the stack. If the top of stack is an input, this becomes a no op.

	stack s;
	input_type_ptr i;

	s.push( "test: test: test: test: test: test: test: test:" );
	s.push( "depth? filter:playlist slots=%s" );

	BOOST_REQUIRE( i = s.pop( ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"0" );
	BOOST_CHECK( s.push( i ).push( L"decap depth?" ).pop( )->get_uri( ) == L"8" );
}

BOOST_AUTO_TEST_CASE( decap_noop )
{
	// Confirms the assertion about the no op in the previous test.

	BOOST_CHECK( stack( ).push( L"test: decap" ).pop( )->get_uri( ) == L"test:" );
}

BOOST_AUTO_TEST_CASE( words )
{
	// A word is defined in the form of a : name ... ; and are basically stack modifiers which can be thought
	// of like functions.

	stack s;
	input_type_ptr i;

	s.push( L": colour colour: r=%s g=%s b=%s ;" );
	s.push( L"1 2 3 colour" );

	BOOST_REQUIRE( i = s.pop( ) );
	BOOST_CHECK( i->property( "r" ).value< int >( ) == 3 );
	BOOST_CHECK( i->property( "g" ).value< int >( ) == 2 );
	BOOST_CHECK( i->property( "b" ).value< int >( ) == 1 );
}

BOOST_AUTO_TEST_CASE( parse )
{
	// parse grabs the next token from the input and puts it on the stack as a string, thus this is equivalent 
	// to the previous test.

	stack s;
	input_type_ptr i;

	s.push( L": colour parse parse parse colour: r=%s g=%s b=%s ;" );
	s.push( L"colour 1 2 3" );

	BOOST_REQUIRE( i = s.pop( ) );
	BOOST_CHECK( i->property( "r" ).value< int >( ) == 3 );
	BOOST_CHECK( i->property( "g" ).value< int >( ) == 2 );
	BOOST_CHECK( i->property( "b" ).value< int >( ) == 1 );
}

BOOST_AUTO_TEST_CASE( redefinition )
{
	// Words can be redefined in the same stack context - let's assume we start with colour as defined in the previous
	// test, but decide that we really want 'colour r g b' instead of 'colour b g r' as it's currently defined - the 
	// most trivial way to do that is change the order of the assignments:

	stack s;
	input_type_ptr i;

	s.push( L": colour parse parse parse colour: r=%s g=%s b=%s ;" );
	s.push( L": colour parse parse parse colour: b=%s g=%s r=%s ;" );
	s.push( L"colour 1 2 3" );

	BOOST_REQUIRE( i = s.pop( ) );
	BOOST_CHECK( i->property( "r" ).value< int >( ) == 1 );
	BOOST_CHECK( i->property( "g" ).value< int >( ) == 2 );
	BOOST_CHECK( i->property( "b" ).value< int >( ) == 3 );
}

BOOST_AUTO_TEST_CASE( parse_fail )
{
	// The problem with parse logic is that the following tokens must be presented in the same token stream,
	// thus all of the following will fail.

	stack s;
	s.push( L": colour parse parse parse colour: r=%s g=%s b=%s ;" );
	BOOST_CHECK_THROW( s.push( L"colour 1 2" ), std::exception );
	BOOST_CHECK_THROW( s.push( L"colour 1 2" ).push( "3" ), std::exception );
}

BOOST_AUTO_TEST_CASE( nesting )
{
	// Words can nest other words - when the word is run, the inner words are created. Thus you can do this kind of 
	// craziness.

	stack s;
	input_type_ptr i;

	s.push( L": pal-sd" );
	s.push( L"  : bg colour: fps_num=25 fps_den=1 width=720 height=576 sar_num=118 sar_den=81 ;" );
	s.push( L";" );

	s.push( L": ntsc-sd" );
	s.push( L"  : bg colour: fps_num=30000 fps_den=1001 width=720 height=480 sar_num=40 sar_den=33 ;" );
	s.push( L";" );

	// before any word is run, bg should not exist
	BOOST_REQUIRE_THROW( i = s.push( L"bg" ).pop( ), std::exception );

	BOOST_REQUIRE( i = s.push( L"ntsc-sd bg" ).pop( ) );
	BOOST_CHECK( i->property( "height" ).value< int >( ) == 480 );

	BOOST_REQUIRE( i = s.push( L"bg" ).pop( ) );
	BOOST_CHECK( i->property( "height" ).value< int >( ) == 480 );

	BOOST_REQUIRE( i = s.push( L"pal-sd bg" ).pop( ) );
	BOOST_CHECK( i->property( "height" ).value< int >( ) == 576 );
}

BOOST_AUTO_TEST_CASE( forget )
{
	// Words can be forgotten

	stack s;
	input_type_ptr i;

	BOOST_CHECK_NO_THROW( s.push( ": something tone: ;" ) );
	BOOST_CHECK_NO_THROW( s.push( "something" ) );
	BOOST_CHECK_NO_THROW( i = s.pop( ) );
	BOOST_REQUIRE( i );
	BOOST_CHECK_NO_THROW( s.push( "forget something" ) );
	BOOST_CHECK_THROW( s.push( "something" ), std::exception );
	BOOST_CHECK_THROW( s.push( "forget something" ), std::exception );
}

BOOST_AUTO_TEST_CASE( comments )
{
	// All content trailing a # is treated as a comment, and is thus ignored by the parser

	stack s;
	BOOST_CHECK_NO_THROW( s.push( "#!/usr/bin/env amlbatch" ) );
	BOOST_CHECK_NO_THROW( s.push( "test: # this is a comment which will be ignored" ) );
	BOOST_CHECK_NO_THROW( s.push( "#test: # this is another comment which will be ignored" ) );
	BOOST_CHECK_NO_THROW( s.pop( ) );
	BOOST_CHECK_THROW( s.pop( ), std::exception );
}

BOOST_AUTO_TEST_CASE( pick )
{
	// pick is a basic FORTH word which takes the form 'n pick' where n is a positive int. It's function
	// is to duplicate the n'th item on the stack (where 0 == top of stack), thus 'test: 0 pick' gives
	// 'test: test:'. However, take note that it's a duplicate and not a clone thus you cannot change 
	// them independently.
	//
	// The FORTH dup word is normally defined as : dup 0 pick ;

	stack s;

	// Check picking any item of an empty stack fails
	BOOST_CHECK_THROW( s.push( "0 pick" ), std::exception );
	BOOST_CHECK_THROW( s.push( "1 pick" ), std::exception );

	// Confirm that we actually have a duplicate and not a clone
	BOOST_CHECK_NO_THROW( s.push( "test:" ) );
	BOOST_CHECK_NO_THROW( s.push( "0 pick" ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"2" );
	BOOST_CHECK( s.pop( ) == s.pop( ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"0" );

	// Confirm that we get the right item
	BOOST_CHECK_NO_THROW( s.push( "colour:" ) );
	BOOST_CHECK_NO_THROW( s.push( "test:" ) );
	BOOST_CHECK_NO_THROW( s.push( "1 pick" ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"3" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"colour:" );
}

BOOST_AUTO_TEST_CASE( roll )
{
	// roll is a basic FORTH word which takes the form 'n roll' where n is a positive int. It's function
	// is to move the n'th item on the stack (where 0 == top of stack) to the top of the stack, thus 
	// 'colour: test: 1 roll' gives 'test: colour:'. 0 roll is a no op when the stack is non-empty.
	//
	// The FORTH swap word is normally defined as : swap 1 roll ;

	stack s;

	// Check rolling any item of an empty stack fails
	BOOST_CHECK_THROW( s.push( "0 roll" ), std::exception );
	BOOST_CHECK_THROW( s.push( "1 roll" ), std::exception );

	// Confirm that 0 roll is a no op
	BOOST_CHECK_NO_THROW( s.push( "test:" ) );
	BOOST_CHECK_NO_THROW( s.push( "0 roll" ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"1" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"test:" );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"0" );

	// Confirm that we get the right item rolled to the top
	BOOST_CHECK_NO_THROW( s.push( "colour:" ) );
	BOOST_CHECK_NO_THROW( s.push( "test:" ) );
	BOOST_CHECK_NO_THROW( s.push( "1 roll" ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"2" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"colour:" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"test:" );
}

BOOST_AUTO_TEST_CASE( drop )
{
	// drop is a basic FORTH word which takes the form 'drop'. It's function is simply to remove the
	// item at the top of the stack.

	stack s;

	// Check drop an item of an empty stack fails
	BOOST_CHECK_THROW( s.push( "drop" ), std::exception );

	// Confirm that drop removes the top of the stack
	BOOST_CHECK_NO_THROW( s.push( "test:" ) );
	BOOST_CHECK_NO_THROW( s.push( "drop" ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"0" );
}

BOOST_AUTO_TEST_CASE( shift )
{
	// shift is a basic FORTH word which takes the form 'n shift' where n is a positive int. It's function
	// is to move the top of the stack to the n'th item on the stack (where 0 == top of stack), thus 
	// '1 2 3 2 shift' gives '3 1 2'. 0 shift is a no op when the stack is non-empty.

	stack s;

	// Check shift of an item of an empty stack fails
	BOOST_CHECK_THROW( s.push( "0 shift" ), std::exception );

	// Confirm that 0 shift is a no op
	BOOST_CHECK_NO_THROW( s.push( "test:" ) );
	BOOST_CHECK_NO_THROW( s.push( "0 shift" ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"1" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"test:" );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"0" );

	// Confirm that we get the right item rolled to the top
	BOOST_CHECK_NO_THROW( s.push( "1 2 3" ) );
	BOOST_CHECK_NO_THROW( s.push( "2 shift" ) );
	BOOST_CHECK( s.push( L"depth?" ).pop( )->get_uri( ) == L"3" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"2" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"1" );
	BOOST_CHECK( s.pop( )->get_uri( ) == L"3" );
}

BOOST_AUTO_TEST_CASE( arithmetic )
{
	// OTT behaviour, but has been useful :)

	stack s;

	BOOST_CHECK( s.push( "1 1 +" ).pop( )->get_uri( ) == L"2" );
	BOOST_CHECK( s.push( "1 0 *" ).pop( )->get_uri( ) == L"0" );
	BOOST_CHECK( s.push( "1 1 -" ).pop( )->get_uri( ) == L"0" );
	BOOST_CHECK( s.push( "1 0 /" ).pop( )->get_uri( ) == L"inf" );
}

BOOST_AUTO_TEST_CASE( empty_release )
{
	// Release will always remove top of stack and ignores underflow

	BOOST_CHECK_NO_THROW( stack( ).release( ) );
}

BOOST_AUTO_TEST_CASE( test_release )
{
	// Releasing something will always return the top of stack or null

	stack s;
	input_type_ptr i;
	BOOST_CHECK_NO_THROW( s.push( "test:" ) );
	BOOST_CHECK_NO_THROW( i = s.release( ) );
	BOOST_CHECK( i->get_uri( ) == L"test:" );
	BOOST_CHECK_NO_THROW( i = s.release( ) );
	BOOST_CHECK( i == input_type_ptr( ) );
}

BOOST_AUTO_TEST_SUITE_END( )
