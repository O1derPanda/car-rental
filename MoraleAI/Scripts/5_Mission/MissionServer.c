modded class MissionServer
{
    private ref array<ref MoraleAISquadManager> m_ActiveSquads;

    override void OnInit()
    {
        super.OnInit();
        m_ActiveSquads = new array<ref MoraleAISquadManager>();

        // Example: Spawn a squad at a specific coordinate for testing
        // You would typically hook this up to an event system or territory trigger
        MoraleAISquadManager squad = new MoraleAISquadManager();
        squad.SpawnSquad("7500 0 7500");
        m_ActiveSquads.Insert(squad);
    }

    override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, target, rpc_type, ctx);

        if (rpc_type == MoraleAIRPC.SERVER_PROCESS_INTERROGATION)
        {
            MoraleAIBotBase bot;
            if (ctx.Read(bot))
            {
                ProcessInterrogation(bot, sender);
            }
        }
    }

    void ProcessInterrogation(MoraleAIBotBase bot, PlayerIdentity sender)
    {
        if (!bot || !bot.IsAlive()) return;

        // Security check: Only allow if tied up and close to player
        if (!bot.IsTiedUp() || bot.IsInterrogated()) return;

        PlayerBase player;
        if (Class.CastTo(player, sender.GetPlayer()))
        {
            if (vector.Distance(player.GetPosition(), bot.GetPosition()) > 5.0) return;
        }
        else
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
            MoraleAIStashManager.GenerateStash(bot.GetPosition(), sender);
        }

        // Kill the bot or just keep them silent
        bot.SetHealth("", "", 0); // Execute bot after interrogation
    }
}
