libdir = $(OPENCORELIB_LIBPATH)

lib_LTLIBRARIES = libopencorelib_cl.la

libopencorelib_cl_la_SOURCES = \
	assembly_class_creator.cpp	\
	assembly_class_creator.hpp	\
	assembly_class_factory.cpp	\
	assembly_class_factory.hpp	\
	assert.cpp					\
	assert_defines.hpp			\
	assert.hpp					\
	assert_persistance.cpp		\
	assert_persistance.hpp		\
	assert_presenter.cpp		\
	assert_presenter.hpp		\
	base_exception.cpp			\
	base_exception.hpp			\
	basic_enums.cpp				\
	basic_enums.hpp				\
	boost_headers.hpp			\
	boost_link_defines.hpp		\
	central_plugin_factory.cpp	\
	central_plugin_factory.hpp	\
	color.cpp					\
	color.hpp					\
	config.hpp					\
	core.cpp					\
	core_enums.cpp				\
	core_enums.hpp				\
	core.hpp					\
	create_selection_dialog.cpp	\
	create_selection_dialog.hpp	\
	deserializer.cpp			\
	deserializer.hpp			\
	detail/plugin_metadata_saxhandler.cpp	\
	detail/plugin_metadata_saxhandler.hpp	\
	disabled_warnings.hpp		\
	doxygen_documentation.hpp	\
	enforce.cpp					\
	enforce_defines.hpp			\
	enforce.hpp					\
	exception_context.cpp		\
	exception_context.hpp		\
	exception_presenter.cpp		\
	exception_presenter.hpp		\
	export_defines.hpp			\
	factory.hpp					\
	frames.cpp					\
	frames.hpp					\
	function_job.cpp			\
	function_job.hpp			\
	guard_define.hpp			\
	initializer.hpp				\
	invoker.hpp				\
	invoker.cpp					\
	ip_address.cpp				\
	ip_address.hpp				\
	jobbase.cpp					\
	jobbase.hpp					\
	local_plugin_factory.hpp	\
	log_defines.hpp				\
	logger.cpp					\
	logger.hpp					\
	loghandler.cpp				\
	loghandler.hpp				\
	logprinter.cpp				\
	logprinter.hpp				\
	logtarget.cpp				\
	logtarget.hpp				\
	machine_readable_errors.hpp	\
	macro_definitions.hpp		\
	media_definitions.cpp		\
	media_definitions.hpp		\
	media_time.cpp				\
	media_time.hpp				\
	message_queue.cpp			\
	message_queue.hpp			\
	minimal_string_defines.hpp	\
	object.cpp					\
	object.hpp					\
	platform_defines.hpp		\
	platform.hpp				\
	plugin_class_base_implementation.cpp	\
	plugin_class_base_implementation.hpp	\
	plugin_loader.cpp			\
	plugin_loader.hpp			\
	plugin_metadata.cpp			\
	plugin_metadata.hpp			\
	point.cpp					\
	point.hpp					\
	precompiled_headers.cpp		\
	precompiled_headers.hpp		\
	property_bag.cpp			\
	property_bag.hpp			\
	rectangle.cpp				\
	rectangle.hpp				\
	selection_dialog.hpp		\
	serializer.hpp				\
	serious_error_handler.cpp	\
	serious_error_handler.hpp	\
	size.cpp					\
	size.hpp					\
	span.cpp					\
	span.hpp					\
	special_folders.cpp			\
	special_folders.hpp			\
	stack_walker.hpp			\
	stream_logtarget.cpp		\
	stream_logtarget.hpp		\
	string_conversions.cpp		\
	string_conversions.hpp		\
	string_defines.hpp			\
	str_util.cpp				\
	str_util.hpp				\
	thread_pool.cpp				\
	thread_pool.hpp				\
	throw_try_catch_defines.hpp	\
	time_code.cpp				\
	time_code.hpp				\
	time_helpers.cpp			\
	time_helpers.hpp			\
	typedefs.hpp				\
	utilities.cpp				\
	utilities.hpp				\
	uuid_16b.cpp				\
	uuid_16b.hpp				\
	worker.cpp					\
	worker.hpp					\
	xerces_sax_handler.cpp		\
	xerces_sax_handler.hpp		\
	xerces_sax_traverser.cpp	\
	xerces_sax_traverser.hpp	\
	xml_sax_parser.cpp			\
	xml_sax_parser.hpp			\
	# end

if HAVE_LINUX

libopencorelib_cl_la_SOURCES += \
	unix/special_folders_unix.cpp\
	# end

else

libopencorelib_cl_la_SOURCES += \
	mac/mac_selection_dlg.cpp	\
	mac/stack_walker.cpp		\
	mac/invoker_impl.cpp		\
	mac/mac_invoker.m			\
	mac/special_folders_impl.cpp\
	mac/special_folders_mac.m	\
	mac/stack_dump.m			\
	#end

endif

libopencorelib_cl_la_CXXFLAGS = \
    $(OLIB_CXXFLAGS) 			\
    $(OLIB_CORE_CXXFLAGS) 		\
    $(BOOST_INCLUDE_PATH) 		\
	-DPREFIX=\"${prefix}\"		\
	-DAMF_VERSION=\"0.0.1\"		\
    # end

libopencorelib_cl_la_LDFLAGS = \
	-lxerces-c -lboost_signals$(BOOST_LIB_TOOLSET)$(BOOST_LIB_RUNTIME)$(BOOST_LIB_VERSION) \
    $(OLIB_LDFLAGS)		\
    $(BOOST_LIB_PATH)   \
	$(OCL_EXTRA_LIBS)	\
    -version-info $(OL_LT_CURRENT):$(OL_LT_REVISION):$(OL_LT_AGE)

library_includedir = $(OPENLIBRARIES_INCLUDEPATH)/opencorelib/cl

library_include_HEADERS =   \
	assembly_class_creator.hpp	\
	assembly_class_factory.hpp	\
	assert_defines.hpp			\
	assert.hpp					\
	assert_persistance.hpp		\
	assert_presenter.hpp		\
	base_exception.hpp			\
	basic_enums.hpp				\
	boost_headers.hpp			\
	boost_link_defines.hpp		\
	central_plugin_factory.hpp	\
	color.hpp					\
	config.hpp					\
	core_enums.hpp				\
	core.hpp					\
	create_selection_dialog.hpp	\
	deserializer.hpp			\
	disabled_warnings.hpp		\
	doxygen_documentation.hpp	\
	enforce_defines.hpp			\
	enforce.hpp					\
	exception_context.hpp		\
	exception_presenter.hpp		\
	export_defines.hpp			\
	factory.hpp					\
	frames.hpp					\
	function_job.hpp			\
	guard_define.hpp			\
	initializer.hpp				\
	invoker.hpp				\
	ip_address.hpp				\
	jobbase.hpp					\
	local_plugin_factory.hpp	\
	log_defines.hpp				\
	logger.hpp					\
	loghandler.hpp				\
	logprinter.hpp				\
	logtarget.hpp				\
	machine_readable_errors.hpp	\
	macro_definitions.hpp		\
	media_definitions.hpp		\
	media_time.hpp				\
	message_queue.hpp			\
	minimal_string_defines.hpp	\
	object.hpp					\
	platform_defines.hpp		\
	platform.hpp				\
	plugin_class_base_implementation.hpp	\
	plugin_loader.hpp			\
	plugin_metadata.hpp			\
	point.hpp					\
	precompiled_headers.hpp		\
	property_bag.hpp			\
	rectangle.hpp				\
	selection_dialog.hpp		\
	serializer.hpp				\
	serious_error_handler.hpp	\
	size.hpp					\
	span.hpp					\
	special_folders.hpp			\
	stack_walker.hpp			\
	stream_logtarget.hpp		\
	string_conversions.hpp		\
	string_defines.hpp			\
	str_util.hpp				\
	thread_pool.hpp				\
	throw_try_catch_defines.hpp	\
	time_code.hpp				\
	time_helpers.hpp			\
	typedefs.hpp				\
	utilities.hpp				\
	uuid_16b.hpp				\
	worker.hpp					\
	xerces_sax_handler.hpp		\
	xerces_sax_traverser.hpp	\
	xml_sax_parser.hpp			\
	# end

AM_OBJCFLAGS = -ObjC++ -I ../../ -DOLIB_USE_UTF8 -fobjc-gc $(AM_CXXFLAGS)
