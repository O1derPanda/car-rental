modded class PlayerBase
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (!GetGame().IsServer()) // Client
        {
            if (rpc_type == MoraleAIRPC.RECEIVE_STASH_WAYPOINT)
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

    void SendMoraleDebugMessage(string msg)
    {
        if (GetGame().IsServer() && GetIdentity())
        {
            ScriptRPC rpc = new ScriptRPC();
            rpc.Write(msg);
            rpc.Send(this, MoraleAIRPC.DEBUG_CHAT_MESSAGE, true, GetIdentity());
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

        string debugMsg = string.Format("[Допрос] Шанс: %1%% | Выпало: %2", chance, roll);

        if (roll <= chance)
        {
            debugMsg += " | Результат: УСПЕХ (Схрон сгенерирован)";
            // Success: Generate Stash
            MoraleAIStashManager.GenerateStash(bot.GetPosition(), this);
        }
        else
        {
            debugMsg += " | Результат: ПРОВАЛ";
        }

        SendMoraleDebugMessage(debugMsg);

        // Removed call to ExecuteBotDeferred so the bot stays alive and tied up.
    }

    // CLIENT
    void ReceiveStashWaypoint(vector wpPos)
    {
        string msg = string.Format("Схрон найден! Координаты: %1, %2", wpPos[0], wpPos[2]);
        GetGame().ChatPlayer(msg);
        Print("[MoraleAI] Received stash waypoint at: " + wpPos.ToString());
    }
}
