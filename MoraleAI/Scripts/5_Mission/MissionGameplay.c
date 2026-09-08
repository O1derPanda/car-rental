modded class MissionGameplay
{
    private ref InterrogationMenu m_InterrogationMenu;


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
