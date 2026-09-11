modded class PlayerBase
{
    private ref InterrogationMenu m_MoraleInterrogationMenu;
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (GetGame().IsServer())
        {
            if (rpc_type == MoraleAIRPC.SERVER_PROCESS_INTERROGATION)
            {
                MoraleAIBotBase bot;
                if (ctx.Read(bot))
                {
                    ProcessInterrogation(bot);
                }
            }
        }
        else // Client
        {
            if (rpc_type == MoraleAIRPC.OPEN_INTERROGATION_GUI)
            {
                MoraleAIBotBase botClient;
                if (ctx.Read(botClient))
                {
                    OpenInterrogationGUI(botClient);
                }
            }
            else if (rpc_type == MoraleAIRPC.RECEIVE_STASH_WAYPOINT)
            {
                vector wpPos;
                if (ctx.Read(wpPos))
                {
                    ReceiveStashWaypoint(wpPos);
                }
            }
            else if (rpc_type == MoraleAIRPC.DEBUG_CHAT_MESSAGE)
            {
                string chatMsg;
                if (ctx.Read(chatMsg))
                {
                    GetGame().ChatPlayer(chatMsg);
                }
            }
        }
    }

    // SERVER
    void ProcessInterrogation(MoraleAIBotBase bot)
    {
        if (!bot || !bot.IsAlive()) return;

        // Security check: Only allow if tied up and not already interrogated
        if (!bot.IsTiedUp() || bot.IsInterrogated()) return;

        // Ensure player is close
        if (vector.Distance(this.GetPosition(), bot.GetPosition()) > 5.0)
        {
            return;
        }

        bot.SetInterrogated(true);

        float chance = 0;
        if (bot.IsLeader())
        {
            chance = Math.RandomFloat(MoraleAIConfig.Get().InterrogationChance_LeaderMin, MoraleAIConfig.Get().InterrogationChance_LeaderMax);
        }
        else
        {
            chance = Math.RandomFloat(MoraleAIConfig.Get().InterrogationChance_ShooterMin, MoraleAIConfig.Get().InterrogationChance_ShooterMax);
        }

        float roll = Math.RandomFloat(0, 100);

        if (roll <= chance)
        {
            // Success: Generate Stash
            MoraleAIStashManager.GenerateStash(bot.GetPosition(), this);
        }

        // Kill the bot or just keep them silent, deferred to prevent physics/network detachment sync conflict
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.ExecuteBotDeferred, 500, false, bot);
    }

    void ExecuteBotDeferred(MoraleAIBotBase bot)
    {
        if (bot && bot.IsAlive())
        {
            bot.SetHealth("", "", 0);
        }
    }

    // CLIENT
    void OpenInterrogationGUI(MoraleAIBotBase bot)
    {
        if (!m_MoraleInterrogationMenu)
        {
            m_MoraleInterrogationMenu = new InterrogationMenu();
        }

        m_MoraleInterrogationMenu.SetBot(bot);

        if (GetGame().GetUIManager().GetMenu() != m_MoraleInterrogationMenu)
        {
            GetGame().GetUIManager().ShowScriptedMenu(m_MoraleInterrogationMenu, NULL);
        }
    }

    void ReceiveStashWaypoint(vector wpPos)
    {
        string msg = string.Format("Схрон найден! Координаты: %1, %2", wpPos[0], wpPos[2]);
        GetGame().ChatPlayer(msg);
        Print("[MoraleAI] Received stash waypoint at: " + wpPos.ToString());
    }
}
