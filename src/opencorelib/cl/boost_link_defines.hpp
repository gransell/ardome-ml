#ifndef _CORE_BOOST_LINK_DEFINES_H_
#define _CORE_BOOST_LINK_DEFINES_H_

#ifndef BOOST_FILESYSTEM_DYN_LINK
#define BOOST_FILESYSTEM_DYN_LINK 1
#endif

#ifndef BOOST_THREAD_DYN_DLL 
#define BOOST_THREAD_DYN_DLL 1
#endif

#ifndef BOOST_DATE_TIME_DYN_LINK
#define BOOST_DATE_TIME_DYN_LINK 1
#endif

#ifndef BOOST_SIGNALS_DYN_LINK
#define BOOST_SIGNALS_DYN_LINK 1
#endif

#ifndef BOOST_SYSTEM_DYN_LINK
#define BOOST_SYSTEM_DYN_LINK 1
#endif


// Don't allow 1.33.1 style usage of boost::filesystem
#define BOOST_FILESYSTEM_NO_DEPRECATED 1

#endif //_CORE_BOOST_LINK_DEFINES_H_
