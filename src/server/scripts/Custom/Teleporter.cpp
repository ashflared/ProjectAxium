#include "ScriptPCH.h"

enum TeleportOption
{
    TELEPORT_MALL = 2,
    TELEPORT_DUEL,
    TELEPORT_GURUBASHI,
};

class Teleporter : public CreatureScript
{
    public:
        Teleporter() : CreatureScript("Teleporter") {}

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->PlayerTalkClass->ClearMenus();

            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Mall", GOSSIP_SENDER_MAIN, TELEPORT_MALL);
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Dueling Zone", GOSSIP_SENDER_MAIN, TELEPORT_DUEL);
            player->SEND_GOSSIP_MENU(1, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
        {
            switch (action)
            {
                case TELEPORT_MALL:
                {
                    break;
                }
                case TELEPORT_DUEL:
                {
                    break;
                }
            }

            return true;
        }
};

void AddSC_Teleporter()
{
    new Teleporter();
}
