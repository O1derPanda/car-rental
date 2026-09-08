modded class MissionServer
{
    private ref array<ref MoraleAISquadManager> m_ActiveSquads;

    override void OnInit()
    {
        super.OnInit();
        m_ActiveSquads = new array<ref MoraleAISquadManager>();

        // Spawn a squad at a specific coordinate for testing
        MoraleAISquadManager squad = new MoraleAISquadManager();
        squad.SpawnSquad("7550 0 7550".ToVector());
        m_ActiveSquads.Insert(squad);
    }

    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId, params);

        // Chat command to respawn bots for testing
        if (eventTypeId == ChatMessageEventTypeID)
        {
            ChatMessageEventParams chatParams;
            if (Class.CastTo(chatParams, params))
            {
                if (chatParams.param3 == "!respawnbots")
                {
                    foreach (MoraleAISquadManager s : m_ActiveSquads)
                    {
                        if (s)
                        {
                            foreach (MoraleAIBotBase bot : s.GetMembers())
                            {
                                if (bot) bot.Delete();
                            }
                        }
                    }
                    m_ActiveSquads.Clear();

                    MoraleAISquadManager newSquad = new MoraleAISquadManager();
                    newSquad.SpawnSquad("7550 0 7550".ToVector());
                    m_ActiveSquads.Insert(newSquad);
                }
            }
        }
    }
}
