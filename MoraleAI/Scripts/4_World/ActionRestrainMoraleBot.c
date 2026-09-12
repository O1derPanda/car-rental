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
        // CMD_ACTIONFB_RESTRAINTARGET requires an active player network target in vanilla DayZ, which causes instant aborts on bots.
        // We use CMD_ACTIONFB_INTERACT which performs a generic continuous animation safely on any object.
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
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
                    // More robust item checking
                    string itemType = item.GetType();
                    itemType.ToLower();

                    if (itemType.Contains("rope") || itemType.Contains("ducttape") || itemType.Contains("handcuffs") || itemType.Contains("metalwire") || item.ConfigGetBool("canRestrain"))
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
        if (bot && action_data.m_MainItem)
        {
            bot.SetRestraintType(action_data.m_MainItem.GetType());
            bot.SetRestraintHealth(action_data.m_MainItem.GetHealth("", ""));

            if (action_data.m_MainItem.HasQuantity())
            {
                bot.SetRestraintQuantity(action_data.m_MainItem.GetQuantity());
            }

            bot.SetTiedUp(true);

            // Spawn the 'Locked' equivalent dummy item into the bot's hands
            // DayZ's AnimGraph automatically handles the restrained animations when these items are in-hands.
            string restraintType = action_data.m_MainItem.GetType();
            string lockedClassname = "";

            if (restraintType == "Rope")
                lockedClassname = "RopeLocked";
            else if (restraintType == "DuctTape")
                lockedClassname = "DuctTapeLocked";
            else if (restraintType == "Handcuffs")
                lockedClassname = "HandcuffsLocked";
            else if (restraintType == "MetalWire")
                lockedClassname = "MetalWireLocked";

            if (lockedClassname != "")
            {
                bot.GetHumanInventory().CreateInHands(lockedClassname);
            }

            // Provide visual feedback that the bot is tied up (kneeling)
            if (bot.GetCommand_Move())
            {
                bot.GetCommand_Move().ForceStance(DayZPlayerConstants.STANCEIDX_CROUCH);
            }

            action_data.m_MainItem.Delete(); // Consume the restraint item
        }
    }
}
