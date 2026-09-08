modded class ActionRestrainTarget
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        // First check if player is holding a restraining item
        if (!item || !item.IsInherited(ItemBase)) return false;

        // Ensure the item is a valid restrain item (Rope, DuctTape, etc.)
        // This is a simplified check, DayZ has specific classes for this but they all inherit from ItemBase
        // We will rely on the base class condition for the item checks, but we need to override the target check

        MoraleAIBotBase bot = MoraleAIBotBase.Cast(target.GetObject());
        if (bot)
        {
            // Only allow restraining if the item is capable of doing so
            // In a fully robust mod, you would check `item.IsKindOf("Rope")` etc.,
            // but relying on standard dayz restrain items is fine if the modded class allows it.
            // However, a simple IsInherited(ItemBase) is too broad. We'll enforce the vanilla check first.

            if (bot.IsAlive() && bot.GetMorale() <= MoraleAIConfig.Get().SurrenderThreshold)
            {
                // DayZ's vanilla ActionCondition for Restrain checks if the target is a player and isn't already restrained.
                // We bypass the player check for our bot, but we must manually enforce that the item is valid.
                if (item.ConfigGetBool("canRestrain") || item.IsInherited(Rope) || item.IsInherited(DuctTape))
                {
                    return true;
                }
            }
            return false;
        }

        // Fallback to standard check for players
        return super.ActionCondition(player, target, item);
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
