class ActionRestrainMoraleBotCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(3.0);
    }
}

class ActionRestrainMoraleBot: ActionContinuousBase
{
    void ActionRestrainMoraleBot()
    {
        m_CallbackClass = ActionRestrainMoraleBotCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_RESTRAIN;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
        m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
    }

    override void CreateConditionComponents()
    {
        m_ConditionTarget = new CCTObject(UAMaxDistances.DEFAULT);
        m_ConditionItem = new CCINonRuined();
    }

    override string GetText()
    {
        return "Tie Up Bot";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        MoraleAIBotBase bot = MoraleAIBotBase.Cast(target.GetObject());
        if (bot && bot.IsAlive())
        {
            if (bot.IsSurrendered() || bot.GetMorale() <= MoraleAIConfig.Get().SurrenderThreshold)
            {
                if (!bot.IsTiedUp())
                {
                    // Check if the item is a valid restrain item (Rope, DuctTape, Handcuffs, etc)
                    if (item.ConfigGetBool("canRestrain") || item.IsInherited(Rope) || item.IsInherited(DuctTape))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        MoraleAIBotBase bot = MoraleAIBotBase.Cast(action_data.m_Target.GetObject());
        if (bot)
        {
            bot.SetTiedUp(true);
            action_data.m_MainItem.Delete(); // Consume the restraint item
        }
    }
}
