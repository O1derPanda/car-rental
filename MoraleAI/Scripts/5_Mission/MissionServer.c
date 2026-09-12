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
            if (!MoraleAIConfig.Get().IsDebugMode)
                return; // Security: Ignore chat commands in production

            ChatMessageEventParams chatParams;
            if (Class.CastTo(chatParams, params))
            {
                // SECURITY WARNING: These chat commands are globally accessible and meant ONLY for local testing.
                // In a production environment, IsDebugMode MUST be false.
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

                    vector spawnPos = "7550 0 7550".ToVector();

                    array<Man> players = new array<Man>;
                    GetGame().GetPlayers(players);

                    if (players && players.Count() > 0)
                    {
                        PlayerBase pb = PlayerBase.Cast(players.Get(0));
                        if (pb)
                        {
                            vector playerPos = pb.GetPosition();
                            vector playerDir = pb.GetDirection();
                            spawnPos = playerPos + (playerDir * 5.0);
                        }
                    }

                    MoraleAISquadManager newSquad = new MoraleAISquadManager();
                    newSquad.SpawnSquad(spawnPos);
                    m_ActiveSquads.Insert(newSquad);
                }
                else if (chatParams.param3 == "!surrender")
                {
                    foreach (MoraleAISquadManager sq : m_ActiveSquads)
                    {
                        if (sq)
                        {
                            foreach (MoraleAIBotBase surrenderBot : sq.GetMembers())
                            {
                                if (surrenderBot && surrenderBot.IsAlive())
                                {
                                    surrenderBot.SetMorale(0); // Instantly trigger surrender state
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
