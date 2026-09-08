modded class MissionGameplay
{
    private ref InterrogationMenu m_InterrogationMenu;

    override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, target, rpc_type, ctx);

        if (rpc_type == MoraleAIRPC.OPEN_INTERROGATION_GUI)
        {
            MoraleAIBotBase bot;
            if (ctx.Read(bot))
            {
                OpenInterrogationGUI(bot);
            }
        }
    }

    void OpenInterrogationGUI(MoraleAIBotBase bot)
    {
        if (!m_InterrogationMenu)
        {
            m_InterrogationMenu = new InterrogationMenu();
        }

        m_InterrogationMenu.SetBot(bot);

        if (GetUIManager().GetMenu() != m_InterrogationMenu)
        {
            GetUIManager().ShowScriptedMenu(m_InterrogationMenu, NULL);
        }
    }
}
