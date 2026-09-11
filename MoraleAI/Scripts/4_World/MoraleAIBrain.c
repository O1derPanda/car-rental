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
        float targetYaw = angles[0];
        float distance = vector.Distance(m_Bot.GetPosition(), m_Target.GetPosition());

        // 2. Obstacle Avoidance (Raycast)
        float finalYaw = targetYaw;
        vector startPos = m_Bot.GetPosition();
        startPos[1] = startPos[1] + 1.0; // Cast from chest height

        // Raycast 3 meters forward
        vector forwardDir = Vector(targetYaw, 0, 0).AnglesToVector();
        vector endPos = startPos + (forwardDir * 3.0);

        vector contactPos;
        vector contactDir;
        int contactComponent;

        // Use DayZPhysics to cast a ray. Ignore the bot itself.
        bool hit = DayZPhysics.RaycastRV(startPos, endPos, contactPos, contactDir, contactComponent, null, null, m_Bot, false, false, ObjIntersectIFire);

        if (hit)
        {
            // If an obstacle is detected directly ahead, steer 90 degrees to the right
            // In a full system, we would cast multiple rays (left, right, forward) to find the best path.
            // For V1, simple right-hand steering.
            finalYaw = targetYaw + 90.0;

            // Normalize yaw to stay within 0-360
            if (finalYaw > 360.0)
                finalYaw -= 360.0;
        }

        // 3. Set Orientation (Face the target or the avoidance path)
        vector newOrientation = Vector(finalYaw, 0, 0);
        m_Bot.SetOrientation(newOrientation);

        // 4. Move using InputController overrides
        // Stop moving if we are within 3 meters AND we are looking at the target
        // (if we are looking away due to avoidance, keep moving to clear the obstacle)
        if (distance > 3.0 || hit)
        {
            inputController.OverrideMovementSpeed(true, 2.0); // Jog
            inputController.OverrideMovementAngle(true, 0.0); // Walk forward relative to current orientation
        }
        else
        {
            inputController.OverrideMovementSpeed(true, 0.0); // Stop
        }
    }
}
