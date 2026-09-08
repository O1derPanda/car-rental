class MoraleAIBotBase extends SurvivorBase
{
    private bool m_IsLeader;
    private float m_Morale;
    private MoraleAIState m_State;
    private ref Timer m_UpdateTimer;
    private bool m_IsTiedUp;
    private bool m_Interrogated;

    void MoraleAIBotBase()
    {
        RegisterNetSyncVariableFloat("m_Morale", 0.0, 100.0);
        RegisterNetSyncVariableBool("m_IsTiedUp");
        RegisterNetSyncVariableBool("m_Interrogated");

        m_Morale = MoraleAIConfig.Get().StartingMorale;
        m_State = MoraleAIState.AGGRESSIVE;
        m_IsTiedUp = false;
        m_Interrogated = false;

        if (GetGame().IsServer())
        {
            m_UpdateTimer = new Timer();
            m_UpdateTimer.Run(1.0, this, "UpdateAI", NULL, true);
        }
    }

    void SetLeader(bool isLeader)
    {
        m_IsLeader = isLeader;

        if (m_IsLeader)
        {
            // Equip visual distinction for leader, e.g. an armband or hat
            // This is a placeholder for actual item spawning
            GetInventory().CreateInInventory("Armband_Red");
        }
    }

    bool IsLeader()
    {
        return m_IsLeader;
    }

    float GetMorale()
    {
        return m_Morale;
    }

    void SetMorale(float morale)
    {
        m_Morale = Math.Clamp(morale, 0.0, MoraleAIConfig.Get().MaxMorale);
        SetSynchDirty();
        UpdateState();
    }

    void UpdateState()
    {
        if (m_State == MoraleAIState.SURRENDER)
            return; // Once surrendered, stays surrendered until tied up or dead

        if (m_Morale <= MoraleAIConfig.Get().SurrenderThreshold)
        {
            m_State = MoraleAIState.SURRENDER;
            OnSurrender();
        }
        else if (m_Morale < 50.0)
        {
            m_State = MoraleAIState.DEFENSIVE;
        }
        else
        {
            m_State = MoraleAIState.AGGRESSIVE;
        }
    }

    void OnSurrender()
    {
        if (GetGame().IsServer())
        {
            // Drop weapon
            EntityAI weapon = GetHumanInventory().GetEntityInHands();
            if (weapon)
            {
                GetInventory().DropEntity(InventoryMode.SERVER, this, weapon);
            }

            // Play surrender animation
            // We use StartCommand_Action to initiate the animation state safely
            StartCommand_Action(DayZPlayerConstants.CMD_ACTIONFB_SURRENDER, 0, 0);

            // Here we would implement the actual AI stop logic
            // (e.g. clear pathfinding, stop shooting)
        }
    }

    void UpdateAI()
    {
        if (!IsAlive() || m_State == MoraleAIState.SURRENDER)
            return;

        // Basic State Machine Logic Placeholder
        switch(m_State)
        {
            case MoraleAIState.AGGRESSIVE:
                // Push player, aggressive logic
                break;
            case MoraleAIState.DEFENSIVE:
                // Seek cover, defensive logic
                break;
        }

        // Morale Regeneration
        RegenerateMorale();
    }

    void RegenerateMorale()
    {
        // Regenerate morale over time (MoraleRegenPerMinute is per 60 seconds, timer is 1 second)
        float regenAmt = MoraleAIConfig.Get().MoraleRegenPerMinute / 60.0;
        SetMorale(GetMorale() + regenAmt);
    }

    private ref MoraleAISquadManager m_Squad;

    void SetSquad(MoraleAISquadManager squad)
    {
        m_Squad = squad;
    }

    override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
    {
        super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

        if (!GetGame().IsServer() || !IsAlive()) return;

        // Apply hit penalty to self
        float penalty = Math.RandomFloat(MoraleAIConfig.Get().Penalty_HitMax, MoraleAIConfig.Get().Penalty_HitMin);
        SetMorale(GetMorale() + penalty);

        // Apply morale penalty to squad members
        if (m_Squad)
        {
            foreach (MoraleAIBotBase member : m_Squad.GetMembers())
            {
                if (member && member != this && member.IsAlive())
                {
                    member.SetMorale(member.GetMorale() + (penalty / 2.0)); // Less penalty for others seeing you get hit
                }
            }
        }
    }

    override void EEKilled(Object killer)
    {
        super.EEKilled(killer);

        if (!GetGame().IsServer()) return;

        if (m_Squad)
        {
            float penalty = 0;
            if (IsLeader())
            {
                penalty = Math.RandomFloat(MoraleAIConfig.Get().Penalty_LeaderDeathMax, MoraleAIConfig.Get().Penalty_LeaderDeathMin);
            }
            else
            {
                penalty = Math.RandomFloat(MoraleAIConfig.Get().Penalty_ShooterDeathMax, MoraleAIConfig.Get().Penalty_ShooterDeathMin);
            }

            foreach (MoraleAIBotBase member : m_Squad.GetMembers())
            {
                if (member && member != this && member.IsAlive())
                {
                    member.SetMorale(member.GetMorale() + penalty);
                }
            }
            m_Squad.RemoveMember(this);
        }
    }

    // Tied up state
    bool IsTiedUp()
    {
        return m_IsTiedUp;
    }

    void SetTiedUp(bool state)
    {
        m_IsTiedUp = state;
        SetSynchDirty();
    }

    bool IsInterrogated()
    {
        return m_Interrogated;
    }

    void SetInterrogated(bool state)
    {
        m_Interrogated = state;
        SetSynchDirty();
    }
}
