#include <vector>
#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/types.hpp>
#include "utils.hpp"

using namespace olib::openmedialib::ml;
using namespace olib::opencorelib::str_util;
using namespace audio;

BOOST_AUTO_TEST_SUITE(audio_convert)

class CoerceCheckTest
{
	std::vector<identity> identities_;

	void init()
	{
		static const identity identities[] = {pcm16_id, pcm24_id, pcm32_id, float_id};
		static const std::size_t ident_size = sizeof(identities) / sizeof(identity);
		identities_.assign(identities, identities + ident_size);
	}

	void coerce_check(identity id) const
	{
		audio_type_ptr source = allocate(id, 48000, 1, 2500, true);

		if (!source) {
			BOOST_REQUIRE(source);
		}

		BOOST_REQUIRE_EQUAL(source->id(), id);

		for (std::size_t i = 0; i < identities_.size(); i++) {
			audio_type_ptr dest = coerce(identities_[i], source);

			if (!dest) {
				BOOST_REQUIRE(dest);
			}

			if (id == identities_[i]) {
				BOOST_REQUIRE_EQUAL(source, dest);
			} else {
				BOOST_REQUIRE(source != dest);
				BOOST_REQUIRE_EQUAL(dest->id(), identities_[i]);
			}
		}
	};

	void check_src_is_float(identity dst_id) const
	{
		audio_type_ptr source = allocate(float_id, 48000, 1, 2, false);
		float          *p     = (float *) source->pointer();

		p[0] = 1.0f;
		p[1] = -1.0f;

		audio_type_ptr d = coerce(dst_id, source);

		if (dst_id == pcm16_id) {
			boost::int16_t *pd = (boost::int16_t *) d->pointer();

			BOOST_REQUIRE_EQUAL(pd[0], boost::integer_traits<short int>::const_max);
			BOOST_REQUIRE_EQUAL(pd[1], boost::integer_traits<short int>::const_min);

		} else if (dst_id == pcm24_id || dst_id == pcm32_id) {
			boost::int32_t *pd = (boost::int32_t *) d->pointer();

			BOOST_REQUIRE_EQUAL(pd[0], boost::integer_traits<int>::const_max);
			BOOST_REQUIRE_EQUAL(pd[1], boost::integer_traits<int>::const_min);

		} else {
			float *pd = (float *) d->pointer();

			BOOST_REQUIRE_EQUAL(pd[0], 1.0f);
			BOOST_REQUIRE_EQUAL(pd[1], -1.0f);
		}
	};

	void check_src_is_pcm16(identity dst_id) const
	{
		audio_type_ptr source = allocate(pcm16_id, 48000, 1, 2, false);
		boost::int16_t *p     = (boost::int16_t *) source->pointer();

		p[0] = boost::integer_traits<short int>::const_max;
		p[1] = boost::integer_traits<short int>::const_min;

		audio_type_ptr d = coerce(dst_id, source);

		if (dst_id == pcm16_id) {
			boost::int16_t *pd = (boost::int16_t *) d->pointer();

			BOOST_REQUIRE_EQUAL(pd[0], p[0]);
			BOOST_REQUIRE_EQUAL(pd[1], p[1]);

		} else if (dst_id == pcm24_id || dst_id == pcm32_id) {
			boost::int32_t *pd = (boost::int32_t *) d->pointer();

			BOOST_REQUIRE_EQUAL(pd[0], 0x7FFF0000);
			BOOST_REQUIRE_EQUAL(pd[1], boost::integer_traits<int>::const_min);

		} else {
			float *pd = (float *) d->pointer();

			BOOST_REQUIRE(pd[0] > 0.999f);
			BOOST_REQUIRE_EQUAL(pd[1], -1.0f);
		}
	};

	void check_src_is_pcm24(identity dst_id) const
	{
		audio_type_ptr source = allocate(pcm24_id, 48000, 1, 2, false);
		boost::int32_t *p     = (boost::int32_t *) source->pointer();

		p[0] = boost::integer_traits<int>::const_max;
		p[1] = boost::integer_traits<int>::const_min;

		audio_type_ptr d = coerce(dst_id, source);

		if (dst_id == pcm16_id) {
			boost::int16_t *pd = (boost::int16_t *) d->pointer();
			BOOST_REQUIRE_EQUAL(pd[0], 0x7FFF);
			BOOST_REQUIRE_EQUAL(pd[1], boost::integer_traits<short int>::const_min);

		} else if (dst_id == pcm24_id || dst_id == pcm32_id) {
			boost::int32_t *pd = (boost::int32_t *) d->pointer();

			BOOST_REQUIRE_EQUAL(pd[0], p[0]);
			BOOST_REQUIRE_EQUAL(pd[1], p[1]);

		} else {
			float *pd = (float *) d->pointer();

			BOOST_REQUIRE(pd[0] > 0.999f);
			BOOST_REQUIRE_EQUAL(pd[1], -1.0f);
		}
	};

	void check_src_is_pcm32(identity dst_id) const
	{
		audio_type_ptr source = allocate(pcm32_id, 48000, 1, 2, false);
		boost::int32_t *p     = (boost::int32_t *) source->pointer();

		p[0] = boost::integer_traits<int>::const_max;
		p[1] = boost::integer_traits<int>::const_min;

		audio_type_ptr d = coerce(dst_id, source);

		if (dst_id == pcm16_id) {
			boost::int16_t *pd = (boost::int16_t *) d->pointer();

			BOOST_REQUIRE_EQUAL(pd[0], 0x7FFF);
			BOOST_REQUIRE_EQUAL(pd[1], boost::integer_traits<short int>::const_min);
		} else if (dst_id == pcm24_id || dst_id == pcm32_id) {
			boost::int32_t *pd = (boost::int32_t *) d->pointer();

			BOOST_REQUIRE_EQUAL(pd[0], p[0]);
			BOOST_REQUIRE_EQUAL(pd[1], p[1]);
		} else {
			float *pd = (float *) d->pointer();

			BOOST_REQUIRE(pd[0] > 0.999f);
			BOOST_REQUIRE_EQUAL(pd[1], -1.0f);
		}
	};

public:
	CoerceCheckTest()
	{
		init();
	}

	void verify() const
	{
		for (std::size_t i = 0; i < identities_.size(); i++) {
			coerce_check(identities_[i]);
			check_src_is_float(identities_[i]);
			check_src_is_pcm16(identities_[i]);
			check_src_is_pcm24(identities_[i]);
			check_src_is_pcm32(identities_[i]);
		}

	};
};

BOOST_AUTO_TEST_CASE(audio_coerce_check)
{
	CoerceCheckTest coerceCheck;
	coerceCheck.verify();
}

BOOST_AUTO_TEST_CASE(flt_values_out_of_boundaries)
{
	audio_type_ptr source  = allocate(float_id, 48000, 1, 2, false);
	float          *p      = (float *)source->pointer();

	p[0] = 1.2f;
	p[1] = -1.2f;

	audio_type_ptr f   = coerce(pcm32_id, source);
	boost::int32_t *pf = (boost::int32_t *)f->pointer();

	BOOST_REQUIRE_EQUAL(pf[0], boost::integer_traits<int>::const_max);
	BOOST_REQUIRE_EQUAL(pf[1], boost::integer_traits<int>::const_min);
}

BOOST_AUTO_TEST_SUITE_END()
