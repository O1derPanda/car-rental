modded class DayZGame
{
    override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, target, rpc_type, ctx);

        if (IsServer())
        {
            if (rpc_type == MoraleAIRPC.SERVER_PROCESS_INTERROGATION)
            {
                MoraleAIBotBase bot;
                if (ctx.Read(bot))
                {
                    // Pass to a handler if needed or process directly
                    // It's cleaner to handle this in MissionServer if we can, but since Mission doesn't have OnRPC,
                    // we can do it here or call a method on GetMission()
                    MissionServer missionSrv = MissionServer.Cast(GetMission());
                    if (missionSrv)
                    {
                        missionSrv.ProcessInterrogation(bot, sender);
                    }
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
                    MissionGameplay missionCl = MissionGameplay.Cast(GetMission());
                    if (missionCl)
                    {
                        missionCl.OpenInterrogationGUI(botClient);
                    }
                }
            }
            else if (rpc_type == MoraleAIRPC.RECEIVE_STASH_WAYPOINT)
            {
                vector wpPos;
                if (ctx.Read(wpPos))
                {
                    MissionGameplay missionClStash = MissionGameplay.Cast(GetMission());
                    if (missionClStash)
                    {
                        missionClStash.ReceiveStashWaypoint(wpPos);
                    }
                }
            }
        }
    }
}
