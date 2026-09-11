class ActionInterrogateBotCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(2.0); // 2 seconds to interrogate
    }
}

class ActionInterrogateBot: ActionContinuousBase
{
    void ActionInterrogateBot()
    {
        m_CallbackClass = ActionInterrogateBotCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINone();
        m_ConditionTarget = new CCTObject(UAMaxDistances.DEFAULT);
    }

    override string GetText()
    {
        return "Interrogate Bot";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        MoraleAIBotBase bot = MoraleAIBotBase.Cast(target.GetObject());
        if (bot && bot.IsAlive() && !bot.IsInterrogated())
        {
            // Check if the bot is in a surrendered state and is tied up
            if (bot.IsTiedUp())
            {
                return true;
            }
        }
        return false;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        MoraleAIBotBase bot = MoraleAIBotBase.Cast(action_data.m_Target.GetObject());
        if (bot)
        {
            PlayerBase player = PlayerBase.Cast(action_data.m_Player);
            if (player)
            {
                player.ProcessInterrogation(bot);
            }
        }
    }
}
