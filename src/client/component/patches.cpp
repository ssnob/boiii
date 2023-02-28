#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include <game/game.hpp>
#include <utils/hook.hpp>

namespace patches
{
	namespace
	{
		void script_errors_stub(const char* file, int line, unsigned int code, const char* fmt, ...)
		{
			char buffer[0x1000];

			{
				va_list ap;
				va_start(ap, fmt);
				vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, ap);
				va_end(ap);
			}

			game::Com_Error(game::ERROR_SCRIPT_DROP, "%s", buffer);
		}
	}

	struct component final : generic_component
	{
		void post_unpack() override
		{
			// don't make script errors fatal error
			utils::hook::call(game::select(0x1412CAC4D, 0x140158EB2), script_errors_stub);
		}
	};
}

REGISTER_COMPONENT(patches::component)
