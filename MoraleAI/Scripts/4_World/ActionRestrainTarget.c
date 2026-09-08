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
            if (bot.IsAlive())
            {
                // For testing/reliability on client: we check if the bot doesn't have a weapon in hands
                // which implies they have dropped it (surrendered). Server validates actual morale.
                bool hasWeapon = false;
                EntityAI weapon = bot.GetHumanInventory().GetEntityInHands();
                if (weapon && weapon.IsWeapon())
                {
                    hasWeapon = true;
                }

                if (!hasWeapon)
                {
                    if (item.ConfigGetBool("canRestrain") || item.IsInherited(Rope) || item.IsInherited(DuctTape))
                    {
                        return true;
                    }
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
