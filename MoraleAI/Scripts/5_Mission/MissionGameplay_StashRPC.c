modded class MissionGameplay
{
    private ref map<vector, Widget> m_StashWaypoints;
    private ref Timer m_WaypointUpdateTimer;
    private Widget m_WaypointLayoutRoot;

    override void OnInit()
    {
        super.OnInit();
        m_StashWaypoints = new map<vector, Widget>();
        m_WaypointLayoutRoot = GetGame().GetWorkspace().CreateWidgets("MoraleAI/GUI/Layouts/WaypointMarker.layout");
        m_WaypointLayoutRoot.Show(false);

        m_WaypointUpdateTimer = new Timer();
        m_WaypointUpdateTimer.Run(0.016, this, "UpdateWaypoints", NULL, true); // Update ~60fps
    }

    override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, target, rpc_type, ctx);

        if (rpc_type == MoraleAIRPC.RECEIVE_STASH_WAYPOINT)
        {
            vector wpPos;
            if (ctx.Read(wpPos))
            {
                Widget wpWidget = GetGame().GetWorkspace().CreateWidgets("MoraleAI/GUI/Layouts/WaypointMarker.layout");
                m_StashWaypoints.Insert(wpPos, wpWidget);
                Print("[MoraleAI] Received stash waypoint at: " + wpPos.ToString());
            }
        }
    }

    void UpdateWaypoints()
    {
        if (!m_StashWaypoints || m_StashWaypoints.Count() == 0) return;

        foreach (vector wp, Widget wpWidget : m_StashWaypoints)
        {
            vector screenPos;
            vector camPos = GetGame().GetCurrentCameraPosition();
            vector camDir = GetGame().GetCurrentCameraDirection();

            // Basic dot product to check if waypoint is in front of the camera
            vector dirToWp = vector.Direction(camPos, wp).Normalized();
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
                        textW.SetText(distance.ToString() + "m");
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
