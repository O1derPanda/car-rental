class MoraleAIBrain
{
    private MoraleAIBotBase m_Bot;
    private PlayerBase m_Target;
    private float m_TimeSinceLastUpdate;

    void MoraleAIBrain(MoraleAIBotBase bot)
    {
        m_Bot = bot;
    }

    void Update(float deltaTime)
    {
        if (!m_Bot || !m_Bot.IsAlive() || m_Bot.IsSurrendered())
            return;

        m_TimeSinceLastUpdate += deltaTime;

        // Update Target Acquisition every 1 second
        if (m_TimeSinceLastUpdate >= 1.0)
        {
            FindTarget();
            m_TimeSinceLastUpdate = 0;
        }

        ProcessCombat();
    }

    private void FindTarget()
    {
        m_Target = NULL;
        float closestDist = 100.0; // Vision radius

        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);

        foreach (Man p : players)
        {
            PlayerBase pb = PlayerBase.Cast(p);
            if (pb && pb.IsAlive() && pb != m_Bot)
            {
                float dist = vector.Distance(m_Bot.GetPosition(), pb.GetPosition());
                if (dist < closestDist)
                {
                    closestDist = dist;
                    m_Target = pb;
                }
            }
        }
    }

    private void ProcessCombat()
    {
        HumanInputController inputController = m_Bot.GetInputController();
        if (!inputController)
            return;

        if (!m_Target || !m_Target.IsAlive())
        {
            m_Target = NULL;
            // Stop movement
            inputController.OverrideMovementSpeed(true, 0.0);
            return;
        }

        // 1. Calculate direction and distance to target using standard vector math
        vector targetDir = (m_Target.GetPosition() - m_Bot.GetPosition()).Normalized();
        vector angles = targetDir.VectorToAngles();
        float yaw = angles[0];
        float distance = vector.Distance(m_Bot.GetPosition(), m_Target.GetPosition());

        // 2. Set Orientation (Face the target)
        // DayZ angles: X is Yaw, Y is Pitch, Z is Roll
        vector newOrientation = Vector(yaw, 0, 0);
        m_Bot.SetOrientation(newOrientation);

        // 3. Move towards target using InputController overrides
        // Stop moving if we are within 3 meters
        if (distance > 3.0)
        {
            // OverrideMovementSpeed(override, speed_value [0=Idle, 1=Walk, 2=Jog, 3=Sprint])
            inputController.OverrideMovementSpeed(true, 2.0); // Jog

            // OverrideMovementAngle(override, angle [0=Forward, 180=Backward, 90/-90=Strafe])
            // Since we snap orientation to face the target, we just walk forward (0)
            inputController.OverrideMovementAngle(true, 0.0);
        }
        else
        {
            inputController.OverrideMovementSpeed(true, 0.0); // Stop
        }
    }
}
