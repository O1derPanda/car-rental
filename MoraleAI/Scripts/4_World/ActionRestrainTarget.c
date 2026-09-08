modded class ActionRestrainTarget
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (super.ActionCondition(player, target, item)) return true;

        MoraleAIBotBase bot = MoraleAIBotBase.Cast(target.GetObject());
        if (bot && bot.IsAlive() && bot.GetMorale() <= MoraleAIConfig.Get().SurrenderThreshold)
        {
            return true;
        }

        return false;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        super.OnFinishProgressServer(action_data);

        MoraleAIBotBase bot = MoraleAIBotBase.Cast(action_data.m_Target.GetObject());
        if (bot)
        {
            bot.SetTiedUp(true);
        }
    }
}
