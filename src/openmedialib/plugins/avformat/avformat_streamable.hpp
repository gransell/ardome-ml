// avformat input whitelisted packet_stream selection
//
// Copyright: Vizrt 2013
// Author   : Daniel Frey
//
// Overview :
//
// Provides a means to control which avformat inputs provide packet streams
// when packet_stream is assigned -1 and whitelist is assigned an appropriate
// xml document of the following basic form:
//
// <whitelist>
//     <version>0</version>
//     <list>
//         <item>
//             <format>mpegts</format>
//             <codec>.*</codec>
//             <index>1</index>
//         </item>
//         <item>
//             <format>^(mov).*</format>
//             <codec>mpeg2video</codec>
//             <index>.*</index>
//         </item>
//     </list>
// </whitelist>

#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace avformat { 

/// A structure for a single whitelist item
struct whitelist_item
{
	/// Regular expression for a video format
	boost::regex format;

	/// Regular expression for a video codec
	boost::regex codec;

	/// Regular expression for an index
	boost::regex index;
};

/// A structure for the whitelist including a method to load it from a xml-file
struct whitelist
{
	/** Loads the whitelist-structure from the specified XML file
		\param inFilePath The path to the XML file.
		\return True, if loaded successfully. False if an error occurred.
	*/
	bool load(const std::string &inFilePath)
	{
		// Only return false from here if no file is specified
		if (inFilePath == "" )
			return false;

		// check if whitelist exists
		ARENFORCE_MSG( boost::filesystem::exists(inFilePath), "The specified whitelist file %s does not exist" )( inFilePath );
		
		// Create an empty property tree object
		using boost::property_tree::ptree;
		ptree pt;

		// Load the XML file into the property tree. If reading fails
		// (cannot open file, parse error), an exception is thrown.
		try
		{
			read_xml(inFilePath, pt);
		}
		catch (boost::property_tree::file_parser_error&)
		{
			ARENFORCE_MSG( false, "The specified whitelist file %s is in error" )( inFilePath );
		}

		// get the main node
		ARENFORCE_MSG( pt.count("whitelist") > 0, "There is no whitelist node present in %s" )( inFilePath );
		ptree& _mainNode = pt.get_child("whitelist");
		
		// parse the file version
		ARENFORCE_MSG( _mainNode.count( "version" ) > 0, "There is no whitelist version present in %s" )( inFilePath );
		mFileVersion = _mainNode.get<int>("version");

		// Iterate over the whitelist items (if there is any)
		if (_mainNode.count("list") != 0)
		{
			BOOST_FOREACH(ptree::value_type &v, _mainNode.get_child("list"))
			{
				// skip no-item children (e.g. comments)
				if (v.first != "item")
				{
					continue;
				}

				try
				{
					// parse the item data
					std::string _format = v.second.get<std::string>("format");
					std::string _codec = v.second.get<std::string>("codec");
					std::string _index = v.second.get<std::string>("index");

					// create regular expressions
					whitelist_item _item;
					_item.format = boost::regex(_format.c_str());
					_item.codec = boost::regex(_codec.c_str());
					_item.index = boost::regex(_index.c_str());

					// add the item to the whitelist
					mList.push_back(_item);			
				}
				catch (boost::property_tree::ptree_error&)
				{
					ARENFORCE_MSG( false, "Parsing a whitelist from %s failed." )( inFilePath );
				}
				catch (boost::regex_error&)
				{
					ARENFORCE_MSG( false, "Parsing a regular expression from %s failed." )( inFilePath );
				}
			}
		}

		return true;
	}

	/// The whitelist file version
	int mFileVersion;

	/// The list of whitelist items
	std::vector<whitelist_item> mList;
};


inline bool is_streamable_old(std::string& inFormat, std::string& inCodec, bool inAmlIndexExists)
{
	bool _result = false;

	_result |= inFormat == "mpegts" && inCodec != "mpeg2video" && inAmlIndexExists;
	_result |= inFormat == "mpeg" && inCodec == "mpeg2video";
	_result |= inFormat == "mxf";
	_result |= inFormat.find( "mov" ) == 0 && inCodec == "mpeg2video";

	return _result;
}

inline bool is_streamable(std::string& inWhitelistPath, std::string& inFormat, std::string& inCodec, bool inAmlIndexExists)
{
	bool _result = false;

	// loads the whitelist from an xml file
	whitelist _whitelist;
	if (_whitelist.load(inWhitelistPath))
	{
		// translate the aml-index-exists bool into a string (for regex matching)
		std::string _amlIndexString = (inAmlIndexExists ? "1" : "0");

		// check the whitelist
		BOOST_FOREACH(whitelist_item& _item, _whitelist.mList)
		{
			// check if the current (format, codec, index) matches with the regexs
			_result |= (
				boost::regex_match(inFormat, _item.format) &&
				boost::regex_match(inCodec, _item.codec) &&
				boost::regex_match(_amlIndexString, _item.index)
			);
		}		
	}
	else
	{
		_result = is_streamable_old(inFormat, inCodec, inAmlIndexExists);
	}

	return _result;
}

}
