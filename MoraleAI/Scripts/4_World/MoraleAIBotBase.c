class MoraleAIBotBase extends PlayerBase
{
    static ref array<MoraleAIBotBase> m_AllBots = new array<MoraleAIBotBase>();

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
    }

    override void EEInit()
    {
        super.EEInit();

        m_AllBots.Insert(this);

        if (GetGame().IsServer())
        {
            m_UpdateTimer = new Timer();
            m_UpdateTimer.Run(1.0, this, "UpdateAI", NULL, true);
        }
    }

    void ~MoraleAIBotBase()
    {
        if (m_AllBots)
        {
            m_AllBots.RemoveItem(this);
        }
    }

    void SetLeader(bool isLeader)
    {
        m_IsLeader = isLeader;

        if (m_IsLeader)
        {
            // Equip visual distinction for leader, e.g. an armband or hat
            EntityAI armband = GetInventory().CreateInInventory("Armband_Red");
            if (!armband)
            {
                GetInventory().CreateAttachment("Armband_Red");
            }
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
            Print("[MoraleAI] Bot has surrendered! Role: " + m_IsLeader.ToString() + " Morale: " + m_Morale.ToString());

            // Drop weapon explicitly
            EntityAI weapon = GetHumanInventory().GetEntityInHands();
            if (weapon)
            {
                ServerDropEntity(weapon);
            }

            // Play surrender animation or fallback to crouch
            if (GetEmoteManager())
            {
                GetEmoteManager().CreateEmoteCBFromMenu(EmoteConstants.ID_EMOTE_SURRENDER);
            }
            else
            {
                GetCommand_Move().ForceStance(DayZPlayerConstants.STANCEIDX_CROUCH);
            }

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

    private MoraleAISquadManager m_Squad;

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

        string dbgMsg = string.Format("[MoraleAI] Bot Hit! Penalty: %1 | New Morale: %2", penalty.ToString(), GetMorale().ToString());
        Print(dbgMsg);
        SendDebugChat(dbgMsg);

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

            string dbgMsg = string.Format("[MoraleAI] Bot Killed! Leader: %1 | Squad Penalty Applied: %2", IsLeader().ToString(), penalty.ToString());
            Print(dbgMsg);
            SendDebugChat(dbgMsg);

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

    void SendDebugChat(string msg)
    {
        if (GetGame().IsServer())
        {
            array<Man> players = new array<Man>;
            GetGame().GetPlayers(players);
            foreach (Man p : players)
            {
                PlayerBase pb = PlayerBase.Cast(p);
                if (pb && pb.GetIdentity())
                {
                    ScriptRPC rpc = new ScriptRPC();
                    rpc.Write(msg);
                    rpc.Send(pb, MoraleAIRPC.DEBUG_CHAT_MESSAGE, true, pb.GetIdentity());
                }
            }
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
