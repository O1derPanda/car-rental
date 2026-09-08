class MoraleAIConfig
{
    // Squad Settings
    int SquadSize = 5;

    // Morale Settings
    float StartingMorale = 80.0;
    float MaxMorale = 100.0;
    float SurrenderThreshold = 15.0;

    // Penalties
    float Penalty_ShooterDeathMin = -10.0;
    float Penalty_ShooterDeathMax = -15.0;
    float Penalty_HitMin = -3.0;
    float Penalty_HitMax = -6.0;
    float Penalty_LeaderDeathMin = -30.0;
    float Penalty_LeaderDeathMax = -40.0;

    // Bonuses
    float Bonus_KillPlayerMin = 10.0;
    float Bonus_KillPlayerMax = 15.0;
    float Bonus_HitPlayerMin = 3.0;
    float Bonus_HitPlayerMax = 5.0;

    // Regeneration
    float MoraleRegenPerMinute = 1.5;

    // Interrogation Chances (Percentage 0-100)
    float InterrogationChance_ShooterMin = 5.0;
    float InterrogationChance_ShooterMax = 7.0;
    float InterrogationChance_LeaderMin = 30.0;
    float InterrogationChance_LeaderMax = 40.0;

    // Stash Generation Settings
    float StashSpawnRadiusMin = 300.0;
    float StashSpawnRadiusMax = 500.0;

    // Non-serialized loaded config instance
    [NonSerialized()]
    private static ref MoraleAIConfig m_Instance;

    static MoraleAIConfig Get()
    {
        if (!m_Instance)
        {
            m_Instance = new MoraleAIConfig();

            // Only server loads/saves from profile, clients use defaults unless synced
            // (RPC sync for config is best practice, but for standalone prototype, defaults are safe)
            if (GetGame() && GetGame().IsServer())
            {
                m_Instance.Load();
            }
        }
        return m_Instance;
    }

    void Load()
    {
        string profilePath = "$profile:MoraleAI/";
        string configPath = profilePath + "config.json";

        if (!FileExist(profilePath))
        {
            MakeDirectory(profilePath);
        }

        if (FileExist(configPath))
        {
            MoraleAIConfig tempConfig;
            JsonFileLoader<MoraleAIConfig>.JsonLoadFile(configPath, tempConfig);

            this.SquadSize = tempConfig.SquadSize;
            this.StartingMorale = tempConfig.StartingMorale;
            this.MaxMorale = tempConfig.MaxMorale;
            this.SurrenderThreshold = tempConfig.SurrenderThreshold;

            this.Penalty_ShooterDeathMin = tempConfig.Penalty_ShooterDeathMin;
            this.Penalty_ShooterDeathMax = tempConfig.Penalty_ShooterDeathMax;
            this.Penalty_HitMin = tempConfig.Penalty_HitMin;
            this.Penalty_HitMax = tempConfig.Penalty_HitMax;
            this.Penalty_LeaderDeathMin = tempConfig.Penalty_LeaderDeathMin;
            this.Penalty_LeaderDeathMax = tempConfig.Penalty_LeaderDeathMax;

            this.Bonus_KillPlayerMin = tempConfig.Bonus_KillPlayerMin;
            this.Bonus_KillPlayerMax = tempConfig.Bonus_KillPlayerMax;
            this.Bonus_HitPlayerMin = tempConfig.Bonus_HitPlayerMin;
            this.Bonus_HitPlayerMax = tempConfig.Bonus_HitPlayerMax;

            this.MoraleRegenPerMinute = tempConfig.MoraleRegenPerMinute;

            this.InterrogationChance_ShooterMin = tempConfig.InterrogationChance_ShooterMin;
            this.InterrogationChance_ShooterMax = tempConfig.InterrogationChance_ShooterMax;
            this.InterrogationChance_LeaderMin = tempConfig.InterrogationChance_LeaderMin;
            this.InterrogationChance_LeaderMax = tempConfig.InterrogationChance_LeaderMax;

            this.StashSpawnRadiusMin = tempConfig.StashSpawnRadiusMin;
            this.StashSpawnRadiusMax = tempConfig.StashSpawnRadiusMax;
        }
        else
        {
            // Save default configuration if it doesn't exist
            Save();
        }
    }

    void Save()
    {
        string profilePath = "$profile:MoraleAI/";
        string configPath = profilePath + "config.json";

        if (!FileExist(profilePath))
        {
            MakeDirectory(profilePath);
        }

        JsonFileLoader<MoraleAIConfig>.JsonSaveFile(configPath, this);
    }
}
