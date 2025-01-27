#include "ChatInfo.h"

#include "../Vars.h"
#include "../Misc/Misc.h"
#include "../../Features/Visuals/Visuals.h"

int attackStringW;
int attackStringH;

static std::string yellow({'\x7', 'C', '8', 'A', '9', '0', '0'}); //C8A900
static std::string blue({'\x7', '0', 'D', '9', '2', 'F', 'F'}); //0D92FF
static std::string white({'\x7', 'F', 'F', 'F', 'F', 'F', 'F'}); //FFFFFF
static std::string red({'\x7', 'F', 'F', '3', 'A', '3', 'A'}); //FF3A3A
static std::string green({'\x7', '3', 'A', 'F', 'F', '4', 'D'}); //3AFF4D

enum VoteLogger {
	VoteText = 1 << 0,
	VoteConsole = 1 << 1,
	VoteChat = 1 << 2,
	VoteParty = 1 << 3,
	VoteVerbose = 1 << 5
};

// Event info
void CChatInfo::Event(CGameEvent* pEvent, const FNV1A_t uNameHash)
{
	if (!I::EngineClient->IsConnected() || !I::EngineClient->IsInGame())
	{
		return;
	}

	if (const auto pLocal = g_EntityCache.GetLocal())
	{
		// Class change
		if ((Vars::Visuals::ChatInfoText.Value || Vars::Visuals::ChatInfoChat.Value) && uNameHash == FNV1A::HashConst("player_changeclass"))
		{
			if (const auto& pEntity = I::ClientEntityList->GetClientEntity(
				I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"))))
			{
				if (pEntity == pLocal) { return; }
				int nIndex = pEntity->GetIndex();

				PlayerInfo_t pi{};
				I::EngineClient->GetPlayerInfo(nIndex, &pi);

				std::string classString = "[FeD] " +
					std::string(pi.name) + " is now a " + std::string(Utils::GetClassByIndex(pEvent->GetInt("class")));
				auto wclassString = std::wstring(classString.begin(), classString.end());

				if (Vars::Visuals::ChatInfoText.Value)
				{
					F::Notifications.Add(classString);
				}

				if (Vars::Visuals::ChatInfoChat.Value)
				{
					const std::string changeClassString(blue + "[FeD] " + red + pi.name + yellow + " is now a " + red + Utils::GetClassByIndex(pEvent->GetInt("class")));
					I::ClientModeShared->m_pChatElement->ChatPrintf(0, changeClassString.c_str());
				}
			}
		}

		// Vote options
		if (Vars::Misc::VotingOptions.Value && uNameHash == FNV1A::HashConst("vote_cast"))
		{
			const auto pEntity = I::ClientEntityList->GetClientEntity(pEvent->GetInt("entityid"));
			if (pEntity && pEntity->IsPlayer())
			{
				const bool bVotedYes = pEvent->GetInt("vote_option") == 0;
				const int votingOptions = Vars::Misc::VotingOptions.Value;
				PlayerInfo_t pi{};
				I::EngineClient->GetPlayerInfo(pEntity->GetIndex(), &pi);
				auto voteLine = tfm::format("[FeD] %s %s voted %s", (pEntity->GetTeamNum() == pLocal->GetTeamNum()) ? "" : "(Enemy)", pi.name, bVotedYes ? "Yes" : "No");

				if (votingOptions & VoteText) // text
				{
					F::Notifications.Add(voteLine);
				}
				if (votingOptions & VoteConsole) // console
				{
					I::Cvar->ConsoleColorPrintf({133, 255, 66, 255}, tfm::format("%s \n", voteLine).c_str());
				}
				if (votingOptions & VoteChat) // chat
				{
					I::ClientModeShared->m_pChatElement->ChatPrintf(0, tfm::format("%s[FeD] \x3%s %svoted %s%s", blue, pi.name, yellow, bVotedYes ? green : red, bVotedYes ? "Yes" : "No").c_str());
				}
				if (votingOptions & VoteParty) // party
				{
					I::EngineClient->ClientCmd_Unrestricted(tfm::format("tf_party_chat \"%s\"", voteLine).c_str());
				}
			}
		}

		// Damage logger
		if ((Vars::Visuals::DamageLoggerConsole.Value || Vars::Visuals::DamageLoggerChat.Value) && uNameHash == FNV1A::HashConst("player_hurt"))
		{
			if (const auto pEntity = I::ClientEntityList->GetClientEntity(
				I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"))))
			{
				const auto nAttacker = pEvent->GetInt("attacker");
				const auto nHealth = pEvent->GetInt("health");
				const auto nDamage = pEvent->GetInt("damageamount");
				const auto bCrit = pEvent->GetBool("crit");
				const int nIndex = pEntity->GetIndex();
				if (pEntity == pLocal) { return; }


				PlayerInfo_t pi{};

				{
					I::EngineClient->GetPlayerInfo(I::EngineClient->GetLocalPlayer(), &pi);
					if (nAttacker != pi.userID) { return; }
				}

				I::EngineClient->GetPlayerInfo(nIndex, &pi);

				const auto maxHealth = pEntity->GetMaxHealth();
				std::string attackString = "You hit " + std::string(pi.name) + " for " + std::to_string(nDamage) + (
					bCrit ? " (crit) " : " ") + "(" + std::to_string(nHealth) + "/" + std::to_string(maxHealth) + ")";

				auto wattackString = std::wstring(attackString.begin(), attackString.end());
				const wchar_t* wcattackString = wattackString.c_str();
				I::VGuiSurface->GetTextSize(g_Draw.m_vecFonts[FONT_ESP_COND].dwFont, wcattackString,
				                        attackStringW, attackStringH);
				const std::string chatAttackString(
					blue + "[FeD]" + yellow + " You hit \x3" + pi.name + yellow + " for " + red + std::to_string(nDamage) + " damage " + (bCrit ? green + "(crit) " : "") + yellow + "(" +
					std::to_string(nHealth) + "/" + std::to_string(maxHealth) + ")");

				if (Vars::Visuals::DamageLoggerChat.Value)
				{
					I::ClientModeShared->m_pChatElement->ChatPrintf(0,
					                                          chatAttackString.c_str());
				}

				if (Vars::Visuals::DamageLoggerConsole.Value)
				{
					I::Cvar->ConsoleColorPrintf({219, 145, 59, 255}, _("%s\n"), attackString.c_str());
				}

				if (Vars::Visuals::DamageLoggerText.Value)
				{
					F::Notifications.Add(attackString);
				}
			}
		}

		// Show Hitboxes on hit
		if (Vars::Aimbot::Global::showHitboxes.Value && uNameHash == FNV1A::HashConst("player_hurt"))
		{
			if (Vars::Aimbot::Global::ClearPreviousHitbox.Value) { I::DebugOverlay->ClearAllOverlays(); }
			auto time = Vars::Aimbot::Global::HitboxLifetime.Value;
			// alpha is how "filled" the hitbox render is, looks bad at anything non-zero (rijin moment)
			auto pEntity = I::ClientEntityList->GetClientEntity(
				I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid")));
			const auto nAttacker = I::ClientEntityList->GetClientEntity(
				I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker")));
			if (pEntity == pLocal) { return; }
			if (pLocal != nAttacker) { return; }
			F::Visuals.DrawHitboxMatrix(pEntity, Colors::HitboxFace, Colors::HitboxEdge, time);
		}

		// Catbot identification (Achievement)
		if (uNameHash == FNV1A::HashConst("achievement_earned"))
		{
			const int player = pEvent->GetInt("player", 0xDEAD);
			const int achievement = pEvent->GetInt("achievement", 0xDEAD);

			// 0xCA7 is an identify and mark request.
			// 0xCA8 is a mark request.

			PlayerInfo_t info{};
			if (I::EngineClient->GetPlayerInfo(player, &info))
			{
				if ((achievement == 0xCA7 || achievement == 0xCA8) && pLocal->GetIndex() != player)
				{
					if (KnownBots.find(info.friendsID) == KnownBots.end())
					{
						F::Notifications.Add(tfm::format("%s is a bot!", info.name));
						if (Vars::Visuals::ChatInfoText.Value)
						{
							I::ClientModeShared->m_pChatElement->ChatPrintf(
								0, tfm::format("%s[FeD]\x3 %s%s is a bot!", blue, info.name, yellow).c_str());
						}

						{
							// marked by other bots. r.i.p cl_drawline :(
							// this will be detected by fedoraware and lmaobox easily.
							// use 0xCA7 if you want to make more bots do the thing,
							// most only care about being marked.
							/*if (Vars::Misc::BeCat.m_Var)
							{
								auto kv = new KeyValues("AchievementEarned");
								kv->SetInt("achievementID", 0xCA8);
								I::Engine->ServerCmdKeyValues(kv);
							}*/
						}

						KnownBots[info.friendsID] = true;
					}
				}
			}
		}
	}

	/*if (uNameHash == FNV1A::HashConst("player_death")) {
		short userid = pEvent->GetInt("userid");									//user ID who died
		long 	victim_entindex = pEvent->GetInt("victim_entindex");
		long 	inflictor_entindex = pEvent->GetInt("inflictor_entindex");			//ent index of inflictor(a sentry, for example)
		short 	attacker = pEvent->GetInt("attacker");								//user ID who killed
		const char* weapon = pEvent->GetString("weapon");							//weapon name killer used
		short 	weaponid = pEvent->GetInt("weaponid"); 								//ID of weapon killed used
		long 	damagebits = pEvent->GetInt("damagebits");							//bits of type of damage
		short 	customkill = pEvent->GetInt("customkill"); 							//type of custom kill
		short 	assister = pEvent->GetInt("assister"); 								//user ID of assister
		const char* weapon_logclassname = pEvent->GetString("weapon_logclassname");	//weapon name that should be printed on the log
		short 	stun_flags = pEvent->GetInt("assister");							//victim's stun flags at the moment of death
		short 	death_flags = pEvent->GetInt("death_flags");						//death flags.
		bool 	silent_kill = pEvent->GetBool("silent_kill");
		short 	playerpenetratecount = pEvent->GetInt("playerpenetratecount");
		const char* assister_fallback = pEvent->GetString("assister_fallback");		//contains a string to use if "assister" is - 1
		short 	kill_streak_total = pEvent->GetInt("kill_streak_total");			//Kill streak count(level)
		short 	kill_streak_wep = pEvent->GetInt("kill_streak_wep");				//Kill streak for killing weapon
		short 	kill_streak_assist = pEvent->GetInt("kill_streak_assist");			//Kill streak for assister count
		short 	kill_streak_victim = pEvent->GetInt("kill_streak_victim");			//Victims kill streak
		short 	ducks_streaked = pEvent->GetInt("ducks_streaked");					//Duck streak increment from this kill
		short 	duck_streak_total = pEvent->GetInt("duck_streak_total");			//Duck streak count for attacker
		short 	duck_streak_assist = pEvent->GetInt("duck_streak_assist");			//Duck streak count for assister
		short 	duck_streak_victim = pEvent->GetInt("duck_streak_victim");			//(former) duck streak count for victim
		bool 	rocket_jump = pEvent->GetBool("rocket_jump");						//was the victim rocket jumping
		short 	weapon_def_index = pEvent->GetInt("weapon_def_index");				//item def index of weapon killer used
		short 	crit_type = pEvent->GetInt("crit_type");							//Crit type of kill. (0: None, 1 : Mini, 2 : Full)
		//I::CVars->ConsolePrintf("weapon: %s", weapon);
		//I::CVars->ConsolePrintf("weapon_logclassname: %s", weapon_logclassname);
		//I::CVars->ConsolePrintf("userid: %d\nvictim_entindex: %d\nattacker: %d\nweapon: %s\nweaponid: %d\ndamagebits: %d\n", userid, victim_entindex, inflictor_entindex, attacker, weapon, weaponid, damagebits);
		//I::CVars->ConsolePrintf("customkill: %d\nassister: %d\nweapon_logclassname: %s\nstun_flags: %d\ndeath_flags: %d\nsilent_kill: %d\nplayerpenetratecount: %d\n", customkill, assister, weapon_logclassname, stun_flags, death_flags, silent_kill, playerpenetratecount);
		//I::CVars->ConsolePrintf("assister_fallback: %s\nkill_streak_total: %d\nkill_streak_wep: %d\nkill_streak_assist: %d\nkill_streak_victim: %d\nducks_streaked: %d\nduck_streak_assist: %d\nduck_streak_victim: %d\n", assister_fallback, kill_streak_total, kill_streak_wep, kill_streak_assist, kill_streak_victim, ducks_streaked, duck_streak_total, duck_streak_assist, duck_streak_victim);
		//I::CVars->ConsolePrintf("rocket_jump: %d\nweapon_def_index: %d\ncrit_type: %d\n", rocket_jump, weapon_def_index, crit_type);
	}*/
}

// User Message info
void CChatInfo::UserMessage(UserMessageType type, bf_read& msgData)
{
	switch (type)
	{
	case VoteStart:
		{
			const int team = msgData.ReadByte();
			const int voteID = msgData.ReadLong();
			const int caller = msgData.ReadByte();
			char reason[64], voteTarget[64];
			msgData.ReadString(reason, 64);
			msgData.ReadString(voteTarget, 64);
			const int target = static_cast<unsigned char>(msgData.ReadByte()) >> 1;

			PlayerInfo_t infoTarget{}, infoCaller{};
			if (const auto& pLocal = g_EntityCache.GetLocal())
			{
				if (target && caller && I::EngineClient->GetPlayerInfo(target, &infoTarget) && I::EngineClient->GetPlayerInfo(caller, &infoCaller))
				{
					const bool bSameTeam = team == pLocal->GetTeamNum();

					const auto bluntLine = tfm::format("%s %s called a vote on %s", bSameTeam ? "" : "(Enemy)", infoCaller.name, infoTarget.name);
					const auto verboseLine = tfm::format("%s %s [U:1:%s] called a vote on %s [U:1:%s]", bSameTeam ? "" : "(Enemy)", infoCaller.name, infoCaller.friendsID, infoTarget.name,
					                                     infoTarget.friendsID);

					const int votingOptions = Vars::Misc::VotingOptions.Value;
					const bool verboseVoting = votingOptions & VoteVerbose;

					const auto chosenLine = verboseVoting ? verboseLine.c_str() : bluntLine.c_str();

					// Log to Text/Notifications
					if (votingOptions & VoteText)
					{
						F::Notifications.Add(chosenLine);
					}

					// Log to Console
					if (votingOptions & VoteConsole)
					{
						I::Cvar->ConsoleColorPrintf({133, 255, 66, 255}, tfm::format("%s \n", chosenLine).c_str());
					}

					// Log to Chat
					if (votingOptions & VoteChat)
					{
						I::ClientModeShared->m_pChatElement->ChatPrintf(0, chosenLine);
					}

					// Log to Party
					if (votingOptions & VoteParty)
					{
						I::EngineClient->ClientCmd_Unrestricted(tfm::format("tf_party_chat \"%s\"", chosenLine).c_str());
					}

					// Auto Vote
					if (Vars::Misc::AutoVote.Value && bSameTeam && target != I::EngineClient->GetLocalPlayer())
					{
						if (G::IsIgnored(infoTarget.friendsID) || g_EntityCache.IsFriend(target))
						{
							I::EngineClient->ClientCmd_Unrestricted(tfm::format("vote %d option2", voteID).c_str()); //f2 on ignored and steam friends
						}
						else
						{
							I::EngineClient->ClientCmd_Unrestricted(tfm::format("vote %d option1", voteID).c_str()); //f1 on everyone else
						}
					}
				}
			}
			break;
		}
	}
}
