class InterrogationMenu extends UIScriptedMenu
{
    private ButtonWidget m_BtnInterrogate;
    private ButtonWidget m_BtnClose;
    private MoraleAIBotBase m_BotTarget;

    void InterrogationMenu()
    {
    }

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("MoraleAI/GUI/Layouts/InterrogationMenu.layout");
        m_BtnInterrogate = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BtnInterrogate"));
        m_BtnClose = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BtnClose"));

        return layoutRoot;
    }

    void SetBot(MoraleAIBotBase bot)
    {
        m_BotTarget = bot;
    }

    override void OnShow()
    {
        super.OnShow();
        GetGame().GetUIManager().ShowCursor(true);
        GetGame().GetInput().ChangeGameFocus(1);
        GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
    }

    override void OnHide()
    {
        super.OnHide();
        GetGame().GetUIManager().ShowCursor(false);
        GetGame().GetInput().ResetGameFocus();
        GetGame().GetMission().PlayerControlEnable(true);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == m_BtnClose)
        {
            Close();
            return true;
        }
        else if (w == m_BtnInterrogate)
        {
            AttemptInterrogation();
            return true;
        }
        return false;
    }

    void AttemptInterrogation()
    {
        if (!m_BotTarget) return;

        // The actual random chance check will be done on the server for security
        ScriptRPC rpc = new ScriptRPC();
        rpc.Write(m_BotTarget);

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        rpc.Send(player, MoraleAIRPC.SERVER_PROCESS_INTERROGATION, true, null);
        Close();
    }
}
