modded class PlayerBase
{
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
                    MissionServer missionSrv = MissionServer.Cast(GetGame().GetMission());
                    if (missionSrv)
                    {
                        missionSrv.ProcessInterrogation(bot, this);
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
                    MissionGameplay missionCl = MissionGameplay.Cast(GetGame().GetMission());
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
                    MissionGameplay missionClStash = MissionGameplay.Cast(GetGame().GetMission());
                    if (missionClStash)
                    {
                        missionClStash.ReceiveStashWaypoint(wpPos);
                    }
                }
            }
        }
    }
}
