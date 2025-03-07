#include "Common.h"
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "TicketMgr.h"
#include "Player.h"
#include "AccountMgr.h"
#include "Opcodes.h"
#include "Chat.h"
#include "Log.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "CellImpl.h"
#include "InstanceSaveMgr.h"
#include "Util.h"
#include "Group.h"

#ifdef _DEBUG_VMAPS
#include "VMapFactory.h"
#endif

bool ChatHandler::HandleNameAnnounceCommand(const char* args)
{
    WorldPacket data;
    if (!*args)
        return false;

    std::string name("Console");
    if (WorldSession* session = GetSession())
        name = session->GetPlayer()->GetName();

    sWorld->SendWorldText(LANG_ANNOUNCE_COLOR, name.c_str(), args);
    return true;
}

bool ChatHandler::HandleGMNameAnnounceCommand(const char* args)
{
    WorldPacket data;
    if (!*args)
        return false;

    std::string name("Console");
    if (WorldSession* session = GetSession())
        name = session->GetPlayer()->GetName();

    sWorld->SendGMText(LANG_GM_ANNOUNCE_COLOR, name.c_str(), args);
    return true;
}

// global announce
bool ChatHandler::HandleAnnounceCommand(const char* args)
{
    if (!*args)
        return false;

    char buff[2048];
    sprintf(buff, GetAxiumString(LANG_SYSTEMMESSAGE), args);
    sWorld->SendServerMessage(SERVER_MSG_STRING, buff);
    return true;
}

// announce to logged in GMs
bool ChatHandler::HandleGMAnnounceCommand(const char* args)
{
    if (!*args)
        return false;

    sWorld->SendGMText(LANG_GM_BROADCAST, args);
    return true;
}

//notification player at the screen
bool ChatHandler::HandleNotifyCommand(const char* args)
{
    if (!*args)
        return false;

    std::string str = GetAxiumString(LANG_GLOBAL_NOTIFY);
    str += args;

    WorldPacket data(SMSG_NOTIFICATION, (str.size()+1));
    data << str;
    sWorld->SendGlobalMessage(&data);

    return true;
}

//notification GM at the screen
bool ChatHandler::HandleGMNotifyCommand(const char* args)
{
    if (!*args)
        return false;

    std::string str = GetAxiumString(LANG_GM_NOTIFY);
    str += args;

    WorldPacket data(SMSG_NOTIFICATION, (str.size()+1));
    data << str;
    sWorld->SendGlobalGMMessage(&data);

    return true;
}

//Summon Player
bool ChatHandler::HandleSummonCommand(const char* args)
{
    Player* target;
    uint64 target_guid;
    std::string target_name;
    if (!extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
        return false;

    Player* _player = m_session->GetPlayer();
    if (target == _player || target_guid == _player->GetGUID())
    {
        PSendSysMessage(LANG_CANT_TELEPORT_SELF);
        SetSentErrorMessage(true);
        return false;
    }

    if (target)
    {
        std::string nameLink = playerLink(target_name);
        // check online security
        if (HasLowerSecurity(target, 0))
            return false;

        if (target->IsBeingTeleported())
        {
            PSendSysMessage(LANG_IS_TELEPORTED, nameLink.c_str());
            SetSentErrorMessage(true);
            return false;
        }

        Map* map = m_session->GetPlayer()->GetMap();

        if (map->IsBattlegroundOrArena())
        {
            if (target->GetBattlegroundId() && m_session->GetPlayer()->GetBattlegroundId() != target->GetBattlegroundId())
                target->LeaveBattleground(false); // Note: should be changed so target gets no Deserter debuff

            // all's well, set bg id
            // when porting out from the bg, it will be reset to 0
            target->SetBattlegroundId(m_session->GetPlayer()->GetBattlegroundId(), m_session->GetPlayer()->GetBattlegroundTypeId());
            // remember current position as entry point for return at bg end teleportation
            if (!target->GetMap()->IsBattlegroundOrArena())
                target->SetBattlegroundEntryPoint();
        }
        else if (map->IsDungeon())
        {
            Map* cMap = target->GetMap();

            if (cMap->Instanceable() && cMap->GetInstanceId() != map->GetInstanceId())
                target->UnbindInstance(map->GetInstanceId(), target->GetDungeonDifficulty(), true);

            // we are in instance, and can summon only player in our group with us as lead
            if (!m_session->GetPlayer()->GetGroup() || !target->GetGroup() ||
                (target->GetGroup()->GetLeaderGUID() != m_session->GetPlayer()->GetGUID()) ||
                (m_session->GetPlayer()->GetGroup()->GetLeaderGUID() != m_session->GetPlayer()->GetGUID()))
                // the last check is a bit excessive, but let it be, just in case
            {
                PSendSysMessage(LANG_CANNOT_SUMMON_TO_INST, nameLink.c_str());
                SetSentErrorMessage(true);
                return false;
            }
        }

        // stop flight if need
        if (target->isInFlight())
        {
            target->GetMotionMaster()->MovementExpired();
            target->CleanupAfterTaxiFlight();
        }
        // save only in non-flight case
        else
            target->SaveRecallPosition();

        // before GM
        float x, y, z;
        m_session->GetPlayer()->GetPosition(x, y, z);
        target->TeleportTo(m_session->GetPlayer()->GetMapId(), x, y, z, target->GetOrientation());
        target->SetPhaseMask(m_session->GetPlayer()->GetPhaseMask(), true);
    }
    else
    {
        // check offline security
        if (HasLowerSecurity(NULL, target_guid))
            return false;

        // in point where GM stay
        Player::SavePositionInDB(m_session->GetPlayer()->GetMapId(),
            m_session->GetPlayer()->GetPositionX(),
            m_session->GetPlayer()->GetPositionY(),
            m_session->GetPlayer()->GetPositionZ(),
            m_session->GetPlayer()->GetOrientation(),
            m_session->GetPlayer()->GetZoneId(),
            target_guid);
    }

    return true;
}

//Teleport to Player
bool ChatHandler::HandleAppearCommand(const char* args)
{
    Player* target;
    uint64 target_guid;
    std::string target_name;
    if (!extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
        return false;

    if (!target)
        return false;

    Player* player = m_session->GetPlayer();
    if (player->CanAppearToTarget(target))
    {
        Map* tMap = target->GetMap();
        if (tMap->IsBattlegroundOrArena())
        {
            if (player->GetBattlegroundId() && player->GetBattlegroundId() != target->GetBattlegroundId())
                player->LeaveBattleground(false);

            player->SetBattlegroundId(target->GetBattlegroundId(), target->GetBattlegroundTypeId());

            if (!player->GetMap()->IsBattlegroundOrArena())
                player->SetBattlegroundEntryPoint();
        }
        else if (tMap->IsDungeon())
        {
            if (player->GetGroup())
                if (player->GetGroup() != target->GetGroup())
                    return false;

            InstancePlayerBind* pBind = player->GetBoundInstance(target->GetMapId(), target->GetDifficulty(tMap->IsRaid()));
            if (!pBind)
            {
                Group* group = player->GetGroup();
                InstanceGroupBind* gBind = group ? group->GetBoundInstance(target) : NULL;

                if (!gBind)
                    if (InstanceSave* save = sInstanceSaveMgr->GetInstanceSave(target->GetInstanceId()))
                        player->BindToInstance(save, !save->CanReset());
            }

            if (tMap->IsRaid())
                player->SetRaidDifficulty(target->GetRaidDifficulty());
            else
                player->SetDungeonDifficulty(target->GetDungeonDifficulty());
        }

        if (player->isInFlight())
        {
            player->GetMotionMaster()->MovementExpired();
            player->CleanupAfterTaxiFlight();
        }
        else
            player->SaveRecallPosition();

        player->TeleportTo(target->GetMapId(), target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation());
        player->SetPhaseMask(target->GetPhaseMask(), true);
        player->lastAppearTime = getMSTime();
    }

    return true;
}

// Teleport player to last position
bool ChatHandler::HandleRecallCommand(const char* args)
{
    Player* target;
    if (!extractPlayerTarget((char*)args, &target))
        return false;

    // check online security
    if (HasLowerSecurity(target, 0))
        return false;

    if (target->IsBeingTeleported())
    {
        PSendSysMessage(LANG_IS_TELEPORTED, GetNameLink(target).c_str());
        SetSentErrorMessage(true);
        return false;
    }

    // stop flight if need
    if (target->isInFlight())
    {
        target->GetMotionMaster()->MovementExpired();
        target->CleanupAfterTaxiFlight();
    }

    target->TeleportTo(target->m_recallMap, target->m_recallX, target->m_recallY, target->m_recallZ, target->m_recallO);
    return true;
}

//Enable On\OFF all taxi paths
bool ChatHandler::HandleTaxiCheatCommand(const char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    std::string argstr = (char*)args;

    Player* chr = getSelectedPlayer();
    if (!chr)
    {
        chr=m_session->GetPlayer();
    }

    // check online security
    else if (HasLowerSecurity(chr, 0))
        return false;

    if (argstr == "on")
    {
        chr->SetTaxiCheater(true);
        PSendSysMessage(LANG_YOU_GIVE_TAXIS, GetNameLink(chr).c_str());
        if (needReportToTarget(chr))
            chr->SendSysMessage(LANG_YOURS_TAXIS_ADDED, GetNameLink().c_str());
        return true;
    }

    if (argstr == "off")
    {
        chr->SetTaxiCheater(false);
        PSendSysMessage(LANG_YOU_REMOVE_TAXIS, GetNameLink(chr).c_str());
        if (needReportToTarget(chr))
            chr->SendSysMessage(LANG_YOURS_TAXIS_REMOVED, GetNameLink().c_str());

        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

bool ChatHandler::HandleLookupAreaCommand(const char* args)
{
    if (!*args)
        return false;

    std::string namepart = args;
    std::wstring wnamepart;

    if (!Utf8toWStr(namepart, wnamepart))
        return false;

    bool found = false;
    uint32 count = 0;
    uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    // Search in AreaTable.dbc
    for (uint32 areaflag = 0; areaflag < sAreaStore.GetNumRows(); ++areaflag)
    {
        AreaTableEntry const* areaEntry = sAreaStore.LookupEntry(areaflag);
        if (areaEntry)
        {
            int loc = GetSessionDbcLocale();
            std::string name = areaEntry->area_name[loc];
            if (name.empty())
                continue;

            if (!Utf8FitTo(name, wnamepart))
            {
                loc = 0;
                for (; loc < TOTAL_LOCALES; ++loc)
                {
                    if (loc == GetSessionDbcLocale())
                        continue;

                    name = areaEntry->area_name[loc];
                    if (name.empty())
                        continue;

                    if (Utf8FitTo(name, wnamepart))
                        break;
                }
            }

            if (loc < TOTAL_LOCALES)
            {
                if (maxResults && count++ == maxResults)
                {
                    PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);
                    return true;
                }

                // send area in "id - [name]" format
                std::ostringstream ss;
                if (m_session)
                    ss << areaEntry->ID << " - |cffffffff|Harea:" << areaEntry->ID << "|h[" << name << ' ' << localeNames[loc]<< "]|h|r";
                else
                    ss << areaEntry->ID << " - " << name << ' ' << localeNames[loc];

                SendSysMessage(ss.str().c_str());

                if (!found)
                    found = true;
            }
        }
    }

    if (!found)
        SendSysMessage(LANG_COMMAND_NOAREAFOUND);

    return true;
}

//Find tele in game_tele order by name
bool ChatHandler::HandleLookupTeleCommand(const char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_COMMAND_TELE_PARAMETER);
        SetSentErrorMessage(true);
        return false;
    }

    char const* str = strtok((char*)args, " ");
    if (!str)
        return false;

    std::string namepart = str;
    std::wstring wnamepart;

    if (!Utf8toWStr(namepart, wnamepart))
        return false;

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    std::ostringstream reply;
    uint32 count = 0;
    uint32 maxResults = sWorld->getIntConfig(CONFIG_MAX_RESULTS_LOOKUP_COMMANDS);
    bool limitReached = false;

    GameTeleMap const & teleMap = sObjectMgr->GetGameTeleMap();
    for (GameTeleMap::const_iterator itr = teleMap.begin(); itr != teleMap.end(); ++itr)
    {
        GameTele const* tele = &itr->second;

        if (tele->wnameLow.find(wnamepart) == std::wstring::npos)
            continue;

        if (maxResults && count++ == maxResults)
        {
            limitReached = true;
            break;
        }

        if (m_session)
            reply << "  |cffffffff|Htele:" << itr->first << "|h[" << tele->name << "]|h|r\n";
        else
            reply << "  " << itr->first << ' ' << tele->name << "\n";
    }

    if (reply.str().empty())
        SendSysMessage(LANG_COMMAND_TELE_NOLOCATION);
    else
        PSendSysMessage(LANG_COMMAND_TELE_LOCATION, reply.str().c_str());

    if (limitReached)
        PSendSysMessage(LANG_COMMAND_LOOKUP_MAX_RESULTS, maxResults);

    return true;
}

//Enable\Dissable accept whispers (for GM)
bool ChatHandler::HandleWhispersCommand(const char* args)
{
    if (!*args)
    {
        PSendSysMessage(LANG_COMMAND_WHISPERACCEPTING, m_session->GetPlayer()->IsAcceptWhispers() ?  GetAxiumString(LANG_ON) : GetAxiumString(LANG_OFF));
        return true;
    }

    std::string argstr = (char*)args;
    // whisper on
    if (argstr == "on")
    {
        m_session->GetPlayer()->SetAcceptWhispers(true);
        SendSysMessage(LANG_COMMAND_WHISPERON);
        return true;
    }

    // whisper off
    if (argstr == "off")
    {
        // Remove all players from the Gamemaster's whisper whitelist
        m_session->GetPlayer()->ClearWhisperWhiteList();
        m_session->GetPlayer()->SetAcceptWhispers(false);
        SendSysMessage(LANG_COMMAND_WHISPEROFF);
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

//Save all players in the world
bool ChatHandler::HandleSaveAllCommand(const char* /*args*/)
{
    sObjectAccessor->SaveAllPlayers();
    SendSysMessage(LANG_PLAYERS_SAVED);
    return true;
}

//Send mail by command
bool ChatHandler::HandleSendMailCommand(const char* args)
{
    // format: name "subject text" "mail text"
    Player* target;
    uint64 target_guid;
    std::string target_name;
    if (!extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
        return false;

    char* tail1 = strtok(NULL, "");
    if (!tail1)
        return false;

    char* msgSubject = extractQuotedArg(tail1);
    if (!msgSubject)
        return false;

    char* tail2 = strtok(NULL, "");
    if (!tail2)
        return false;

    char* msgText = extractQuotedArg(tail2);
    if (!msgText)
        return false;

    // msgSubject, msgText isn't NUL after prev. check
    std::string subject = msgSubject;
    std::string text    = msgText;

    // from console show not existed sender
    MailSender sender(MAIL_NORMAL, m_session ? m_session->GetPlayer()->GetGUIDLow() : 0, MAIL_STATIONERY_GM);

    //- TODO: Fix poor design
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    MailDraft(subject, text)
        .SendMailTo(trans, MailReceiver(target, GUID_LOPART(target_guid)), sender);

    CharacterDatabase.CommitTransaction(trans);

    std::string nameLink = playerLink(target_name);
    PSendSysMessage(LANG_MAIL_SENT, nameLink.c_str());
    return true;
}
