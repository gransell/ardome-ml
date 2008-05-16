#include "precompiled_headers.hpp"
#include "./enforce.hpp"
#include "./enforce_defines.hpp"
#include "./str_util.hpp"
#include "./central_plugin_factory.hpp"
#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

namespace olib
{

	namespace opencorelib
	{
          typedef Loki::SingletonHolder< plugin_loader > the_internal_plugin_loader;

          plugin_loader& the_plugin_loader::instance()
          {
              return the_internal_plugin_loader::Instance();
          }
	}
}
