modded class MissionServer
{
    private ref array<ref MoraleAISquadManager> m_ActiveSquads;

    override void OnInit()
    {
        super.OnInit();
        m_ActiveSquads = new array<ref MoraleAISquadManager>();

        // Spawn a squad at a specific coordinate for testing
        MoraleAISquadManager squad = new MoraleAISquadManager();
        squad.SpawnSquad("7500 0 7500".ToVector());
        m_ActiveSquads.Insert(squad);
    }
}
