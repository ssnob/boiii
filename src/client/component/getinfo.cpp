#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"
#include "steam/steam.hpp"

#include "network.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/info_string.hpp>

#include <version.hpp>

#include "game/utils.hpp"

namespace getinfo
{
	int get_max_client_count()
	{
		return game::get_dvar_int("com_maxclients");
	}

	int get_client_count()
	{
		int count = 0;
		const auto client_states = *reinterpret_cast<uint64_t*>(game::select(0x1576FB318, 0x14A178E98));
		if (!client_states)
		{
			return 0;
		}

		const auto object_length = game::is_server() ? 0xE5110 : 0xE5170;

		for (int i = 0; i < get_max_client_count(); ++i)
		{
			const auto client_state = *reinterpret_cast<int*>(client_states + (i * object_length));
			if (client_state > 0)
			{
				++count;
			}
		}

		return count;
	}

	int get_bot_count()
	{
		const auto client_states = *reinterpret_cast<uint64_t*>(game::select(0x1576FB318, 0x14A178E98));
		if (!client_states)
		{
			return 0;
		}

		int count = 0;

		for (int i = 0; i < get_max_client_count(); ++i)
		{
			if (game::SV_IsTestClient(i))
			{
				++count;
			}
		}

		return count;
	}

	namespace
	{
		int Com_SessionMode_GetGameMode()
		{
			return *reinterpret_cast<int*>(game::select(0x1568EF7F4, 0x14948DB04)) << 14 >> 28;
		}
	}

	int get_assigned_team()
	{
		return (rand() % 2) + 1;
	}

	bool is_host()
	{
		return game::SV_Loaded() && (game::is_server() || !game::Com_IsRunningUILevel());
	}

	struct component final : generic_component
	{
		void post_unpack() override
		{
			//utils::hook::jump(game::select(0x142254EF0, 0x140537730), get_assigned_team);

			network::on("getInfo", [](const game::netadr_t& target, const network::data_view& data)
			{
				utils::info_string info{};
				info.set("challenge", std::string(data.begin(), data.end()));
				info.set("gamename", "T7");
				info.set("hostname",
				         game::get_dvar_string(game::is_server() ? "live_steam_server_name" : "sv_hostname"));
				info.set("gametype", game::get_dvar_string("g_gametype"));
				//info.set("sv_motd", get_dvar_string("sv_motd"));
				info.set("description",
				         game::is_server() ? game::get_dvar_string("live_steam_server_description") : "");
				info.set("xuid", utils::string::va("%llX", steam::SteamUser()->GetSteamID().bits));
				info.set("mapname", game::get_dvar_string("mapname"));
				info.set("isPrivate", game::get_dvar_string("g_password").empty() ? "0" : "1");
				info.set("clients", utils::string::va("%i", get_client_count()));
				info.set("bots", utils::string::va("%i", get_bot_count()));
				info.set("sv_maxclients", utils::string::va("%i", get_max_client_count()));
				info.set("protocol", utils::string::va("%i", PROTOCOL));
				info.set("playmode", utils::string::va("%i", game::Com_SessionMode_GetMode()));
				info.set("gamemode", utils::string::va("%i", Com_SessionMode_GetGameMode()));
				//info.set("sv_running", utils::string::va("%i", get_dvar_bool("sv_running")));
				info.set("dedicated", utils::string::va("%i", game::is_server() ? 1 : 0));
				info.set("shortversion", SHORTVERSION);

				network::send(target, "infoResponse", info.build(), '\n');
			});
		}
	};
}

REGISTER_COMPONENT(getinfo::component)
