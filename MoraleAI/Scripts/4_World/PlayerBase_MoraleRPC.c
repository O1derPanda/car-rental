modded class PlayerBase
{
    private ref InterrogationMenu m_MoraleInterrogationMenu;
    private ref array<ref MoraleAIWaypoint> m_StashWaypoints;
    private ref Timer m_WaypointUpdateTimer;

    // Debug UI
    private Widget m_DebugRootWidget;
    private MultilineTextWidget m_DebugTextWidget;
    private ref Timer m_DebugUpdateTimer;

    override void EEInit()
    {
        super.EEInit();

        if (GetGame().IsClient())
        {
            if (IsControlledPlayer())
            {
                m_StashWaypoints = new array<ref MoraleAIWaypoint>();
                m_WaypointUpdateTimer = new Timer();
                m_WaypointUpdateTimer.Run(0.016, this, "UpdateWaypoints", NULL, true);

                m_DebugUpdateTimer = new Timer();
                m_DebugUpdateTimer.Run(0.5, this, "UpdateDebugPanel", NULL, true);
            }
        }
    }

    void InitDebugPanel()
    {
        if (!m_DebugRootWidget)
        {
            m_DebugRootWidget = GetGame().GetWorkspace().CreateWidgets("MoraleAI/GUI/Layouts/MoraleAIDebug.layout");
            m_DebugTextWidget = MultilineTextWidget.Cast(m_DebugRootWidget.FindAnyWidget("DebugText"));
        }
    }

    void UpdateDebugPanel()
    {
        if (!IsControlledPlayer()) return;
        InitDebugPanel();

        if (!m_DebugTextWidget) return;

        string debugStr = "Nearby Bots:\n\n";
        int botCount = 0;

        array<Object> objects = new array<Object>;
        array<CargoBase> proxyCargos = new array<CargoBase>;
        GetGame().GetObjectsAtPosition(GetPosition(), 50.0, objects, proxyCargos);

        foreach (Object obj : objects)
        {
            MoraleAIBotBase bot = MoraleAIBotBase.Cast(obj);
            if (bot)
            {
                botCount++;
                string role = "Shooter";
                if (bot.IsLeader()) role = "Leader";

                string stateStr = "Alive";
                if (!bot.IsAlive()) stateStr = "Dead";
                else if (bot.IsTiedUp()) stateStr = "Tied Up";
                else if (bot.GetMorale() <= MoraleAIConfig.Get().SurrenderThreshold) stateStr = "Surrendered";

                debugStr += string.Format("[%1] Morale: %2 | State: %3\n", role, bot.GetMorale().ToString(), stateStr);
            }
        }

        if (botCount == 0)
        {
            debugStr += "No bots in 50m radius.";
        }

        m_DebugTextWidget.SetText(debugStr);
    }

    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (GetGame().IsServer())
        {
            if (rpc_type == MoraleAIRPC.SERVER_PROCESS_INTERROGATION)
            {
                MoraleAIBotBase bot;
                if (ctx.Read(bot))
                {
                    ProcessInterrogation(bot);
                }
            }
        }
        else // Client
        {
            if (rpc_type == MoraleAIRPC.OPEN_INTERROGATION_GUI)
            {
                MoraleAIBotBase botClient;
                if (ctx.Read(botClient))
                {
                    OpenInterrogationGUI(botClient);
                }
            }
            else if (rpc_type == MoraleAIRPC.RECEIVE_STASH_WAYPOINT)
            {
                vector wpPos;
                if (ctx.Read(wpPos))
                {
                    ReceiveStashWaypoint(wpPos);
                }
            }
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

        if (roll <= chance)
        {
            // Success: Generate Stash
            MoraleAIStashManager.GenerateStash(bot.GetPosition(), this);
        }

        // Kill the bot or just keep them silent
        bot.SetHealth("", "", 0); // Execute bot after interrogation
    }

    // CLIENT
    void OpenInterrogationGUI(MoraleAIBotBase bot)
    {
        if (!m_MoraleInterrogationMenu)
        {
            m_MoraleInterrogationMenu = new InterrogationMenu();
        }

        m_MoraleInterrogationMenu.SetBot(bot);

        if (GetGame().GetUIManager().GetMenu() != m_MoraleInterrogationMenu)
        {
            GetGame().GetUIManager().ShowScriptedMenu(m_MoraleInterrogationMenu, NULL);
        }
    }

    void ReceiveStashWaypoint(vector wpPos)
    {
        Widget wpWidget = GetGame().GetWorkspace().CreateWidgets("MoraleAI/GUI/Layouts/WaypointMarker.layout");
        m_StashWaypoints.Insert(new MoraleAIWaypoint(wpPos, wpWidget));
        Print("[MoraleAI] Received stash waypoint at: " + wpPos.ToString());
    }

    void UpdateWaypoints()
    {
        if (!m_StashWaypoints || m_StashWaypoints.Count() == 0) return;
        if (!IsControlledPlayer()) return;

        for (int i = 0; i < m_StashWaypoints.Count(); i++)
        {
            MoraleAIWaypoint waypoint = m_StashWaypoints.Get(i);
            if (!waypoint || !waypoint.widget) continue;

            vector wp = waypoint.position;
            Widget wpWidget = waypoint.widget;

            vector screenPos;
            vector camPos = GetGame().GetCurrentCameraPosition();
            vector camDir = GetGame().GetCurrentCameraDirection();

            // Basic dot product to check if waypoint is in front of the camera
            vector dirToWp = (wp - camPos).Normalized();
            if (vector.Dot(camDir, dirToWp) > 0)
            {
                screenPos = GetGame().GetScreenPosRelative(wp);

                // Draw layout widget on screen if it's on screen
                if (screenPos[0] > 0 && screenPos[0] < 1 && screenPos[1] > 0 && screenPos[1] < 1)
                {
                    wpWidget.Show(true);
                    wpWidget.SetPos(screenPos[0], screenPos[1], true);

                    TextWidget textW = TextWidget.Cast(wpWidget.FindAnyWidget("DistanceText"));
                    if (textW)
                    {
                        float distance = vector.Distance(camPos, wp);
                        textW.SetText(Math.Round(distance).ToString() + "m");
                    }
                }
                else
                {
                    wpWidget.Show(false);
                }
            }
            else
            {
                wpWidget.Show(false);
            }
        }
    }
}
