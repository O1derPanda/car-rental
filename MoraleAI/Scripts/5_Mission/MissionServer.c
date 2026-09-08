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
        squad.SpawnSquad("7500 0 7500".ToVector());
        m_ActiveSquads.Insert(squad);
    }


    void ProcessInterrogation(MoraleAIBotBase bot, PlayerBase player)
    {
        if (!bot || !bot.IsAlive()) return;

        // Security check: Only allow if tied up and close to player
        if (!bot.IsTiedUp() || bot.IsInterrogated()) return;

        if (!player || vector.Distance(player.GetPosition(), bot.GetPosition()) > 5.0)
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
            MoraleAIStashManager.GenerateStash(bot.GetPosition(), player.GetIdentity());
        }

        // Kill the bot or just keep them silent
        bot.SetHealth("", "", 0); // Execute bot after interrogation
    }
}
