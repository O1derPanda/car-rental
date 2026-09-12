class MoraleAIBrain
{
    private MoraleAIBotBase m_Bot;
    private PlayerBase m_Target;
    private float m_TimeSinceLastUpdate;

    // Pathfinding
    private ref array<vector> m_Path;
    private int m_CurrentWaypointIndex;
    private float m_PathfindTimer;
    private ref PGFilter m_PathFilter;

    void MoraleAIBrain(MoraleAIBotBase bot)
    {
        m_Bot = bot;
        m_Path = new array<vector>();

        // Initialize Pathfinding Filter (allows NavMesh usage)
        m_PathFilter = new PGFilter();
        m_PathFilter.SetFlags(PGPolyFlags.WALK | PGPolyFlags.DOOR | PGPolyFlags.INSIDE, PGPolyFlags.NONE, PGPolyFlags.NONE);
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
            inputController.OverrideMovementSpeed(true, 0.0);
            return;
        }

        float distanceToTarget = vector.Distance(m_Bot.GetPosition(), m_Target.GetPosition());

        // Update Path every 1 second
        m_PathfindTimer += 0.1; // Function called at 10Hz
        if (m_PathfindTimer >= 1.0)
        {
            m_PathfindTimer = 0;
            UpdatePathfinding(m_Target.GetPosition());
        }

        // Move along the path
        if (m_Path && m_Path.Count() > 0 && m_CurrentWaypointIndex < m_Path.Count())
        {
            vector currentWaypoint = m_Path.Get(m_CurrentWaypointIndex);

            // Ignore Y for distance checks to prevent getting stuck on slight elevations
            vector botPosFlat = m_Bot.GetPosition(); botPosFlat[1] = 0;
            vector wpFlat = currentWaypoint; wpFlat[1] = 0;

            float distToWaypoint = vector.Distance(botPosFlat, wpFlat);

            // If we reached the waypoint, move to the next
            if (distToWaypoint < 0.5)
            {
                m_CurrentWaypointIndex++;
                if (m_CurrentWaypointIndex >= m_Path.Count())
                {
                    // Reached end of path
                    m_Path.Clear();
                    inputController.OverrideMovementSpeed(true, 0.0);
                    return;
                }
                currentWaypoint = m_Path.Get(m_CurrentWaypointIndex);
            }

            // Calculate direction to the current NavMesh waypoint
            vector dirToWp = (currentWaypoint - m_Bot.GetPosition()).Normalized();
            vector angles = dirToWp.VectorToAngles();
            float yaw = angles[0];

            m_Bot.SetOrientation(Vector(yaw, 0, 0));

            // Dynamic Speed / Posture logic based on distance to the ultimate target
            if (distanceToTarget > 20.0)
            {
                inputController.OverrideMovementSpeed(true, 3.0); // Sprint
            }
            else if (distanceToTarget > 5.0)
            {
                inputController.OverrideMovementSpeed(true, 2.0); // Jog
            }
            else if (distanceToTarget <= 5.0 && distanceToTarget > 3.0)
            {
                inputController.OverrideMovementSpeed(true, 1.0); // Walk
            }
            else
            {
                inputController.OverrideMovementSpeed(true, 0.0); // Stop/Shoot
            }

            inputController.OverrideMovementAngle(true, 0.0); // Walk forward
        }
        else
        {
            // No path, just turn to target and stop (fallback)
            vector fallbackDir = (m_Target.GetPosition() - m_Bot.GetPosition()).Normalized();
            vector fallbackAngles = fallbackDir.VectorToAngles();
            m_Bot.SetOrientation(Vector(fallbackAngles[0], 0, 0));
            inputController.OverrideMovementSpeed(true, 0.0);
        }
    }

    private void UpdatePathfinding(vector targetPos)
    {
        m_Path.Clear();
        m_CurrentWaypointIndex = 0;

        if (GetGame() && GetGame().GetWorld() && GetGame().GetWorld().GetAIWorld())
        {
            // Enfusion native NavMesh pathfinding
            GetGame().GetWorld().GetAIWorld().FindPath(m_Bot.GetPosition(), targetPos, m_PathFilter, m_Path);
        }
    }
}
