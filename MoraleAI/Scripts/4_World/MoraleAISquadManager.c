class MoraleAISquadManager
{
    private ref array<MoraleAIBotBase> m_SquadMembers;
    private MoraleAIBotBase m_Leader;

    void MoraleAISquadManager()
    {
        m_SquadMembers = new array<MoraleAIBotBase>();
    }

    void SpawnSquad(vector position)
    {
        if (!GetGame().IsServer())
            return;

        int squadSize = MoraleAIConfig.Get().SquadSize;

        for (int i = 0; i < squadSize; i++)
        {
            vector spawnPos = position;
            if (i > 0)
            {
                spawnPos[0] = spawnPos[0] + Math.RandomFloat(-3.0, 3.0);
                spawnPos[2] = spawnPos[2] + Math.RandomFloat(-3.0, 3.0);
                spawnPos[1] = GetGame().SurfaceY(spawnPos[0], spawnPos[2]);
            }

            // Spawn bot (using our custom class)
            MoraleAIBotBase bot = MoraleAIBotBase.Cast(GetGame().CreateObject("MoraleAIBotBase", spawnPos));
            if (bot)
            {
                m_SquadMembers.Insert(bot);

                // Assign leader (first spawned)
                if (i == 0)
                {
                    m_Leader = bot;
                    bot.SetLeader(true);
                }
                else
                {
                    bot.SetLeader(false);
                }

                // Equip basic gear
                bot.GetInventory().CreateInInventory("TShirt_Green");
                bot.GetInventory().CreateInInventory("Jeans_Blue");
                bot.GetInventory().CreateInInventory("TaloonBag_Green");

                EntityAI weapon = EntityAI.Cast(bot.GetHumanInventory().CreateInHands("M4A1"));
                if (weapon)
                {
                    weapon.GetInventory().CreateAttachment("Mag_STANAG_30Rnd");
                }
            }
        }
    }

    array<MoraleAIBotBase> GetMembers()
    {
        return m_SquadMembers;
    }

    void RemoveMember(MoraleAIBotBase bot)
    {
        m_SquadMembers.RemoveItem(bot);
    }
}
