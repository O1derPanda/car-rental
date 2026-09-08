modded class PlayerBase
{
    override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
    {
        super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

        if (!GetGame().IsServer()) return;

        // If a bot hit the player, give the bot morale
        if (source && source.IsInherited(MoraleAIBotBase))
        {
            MoraleAIBotBase bot = MoraleAIBotBase.Cast(source);
            float bonus = Math.RandomFloat(MoraleAIConfig.Get().Bonus_HitPlayerMin, MoraleAIConfig.Get().Bonus_HitPlayerMax);
            bot.SetMorale(bot.GetMorale() + bonus);
        }
    }

    override void EEKilled(Object killer)
    {
        super.EEKilled(killer);

        if (!GetGame().IsServer()) return;

        // If a bot killed the player, give the bot morale
        if (killer && killer.IsInherited(MoraleAIBotBase))
        {
            MoraleAIBotBase bot = MoraleAIBotBase.Cast(killer);
            float bonus = Math.RandomFloat(MoraleAIConfig.Get().Bonus_KillPlayerMin, MoraleAIConfig.Get().Bonus_KillPlayerMax);
            bot.SetMorale(bot.GetMorale() + bonus);
        }
    }
}
