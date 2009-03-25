#include "precompiled_headers.hpp"
#include "./stream_logtarget.hpp"

void olib::opencorelib::stream_logtarget::log(olib::opencorelib::invoke_assert& a, const TCHAR* log_source) {
    m_out << a << std::endl;
}

void olib::opencorelib::stream_logtarget::log(olib::opencorelib::base_exception& e, const TCHAR* log_source) {
    e.pretty_print_one_line(m_out, olib::opencorelib::print::output_default);
    m_out << std::endl;
}

void olib::opencorelib::stream_logtarget::log(olib::opencorelib::logger& log_msg, const TCHAR* log_source) {
    log_msg.pretty_print_one_line(m_out, olib::opencorelib::print::output_default);
    m_out << std::endl;
}
