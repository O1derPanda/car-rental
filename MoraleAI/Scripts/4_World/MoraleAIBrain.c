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

    // Stamina
    private float m_Stamina;
    private bool m_IsExhausted;
    private float m_DoorCheckTimer;

    // Anti-Stuck Logic
    private vector m_LastPos;
    private float m_StuckTimer;
    private bool m_IsEvading;

    void MoraleAIBrain(MoraleAIBotBase bot)
    {
        m_Bot = bot;
        m_Path = new array<vector>();
        m_Stamina = 100.0;
        m_IsExhausted = false;
        m_DoorCheckTimer = 0.0;
        m_StuckTimer = 0.0;
        m_IsEvading = false;
        m_LastPos = bot.GetPosition();

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

            // Stamina & Speed Logic (Calculate desired speed FIRST)
            float desiredSpeed = 0.0;

            if (m_Stamina <= 10.0)
            {
                m_IsExhausted = true;
            }
            else if (m_Stamina >= 80.0)
            {
                m_IsExhausted = false;
            }

            // Combat range (stop if visible and within 30m, but for now we stop closer to test nav)
            if (distanceToTarget <= 10.0)
            {
                desiredSpeed = 0.0; // Stop
                m_Stamina += 1.0;
            }
            else if (distanceToTarget > 30.0)
            {
                if (!m_IsExhausted)
                {
                    desiredSpeed = 3.0; // Sprint
                    m_Stamina -= 1.0; // Drain
                }
                else
                {
                    desiredSpeed = 2.0; // Jog (recovering)
                    m_Stamina += 0.5;
                }
            }
            else
            {
                desiredSpeed = 2.0; // Jog
                m_Stamina += 0.5; // Slow regen
            }

            // Calculate direction to the current NavMesh waypoint
            vector dirToWp = (currentWaypoint - m_Bot.GetPosition()).Normalized();
            vector angles = dirToWp.VectorToAngles();
            float yaw = angles[0];

            float movementAngle = 0.0; // 0 is forward

            // Handle Anti-Stuck Logic ONLY if the bot is actually trying to move
            if (desiredSpeed > 0.0)
            {
                float distMoved = vector.Distance(m_Bot.GetPosition(), m_LastPos);
                if (distMoved < 0.05) // Barely moved in 0.1s
                {
                    m_StuckTimer += 0.1;
                }
                else
                {
                    m_StuckTimer = 0.0;
                    m_IsEvading = false;
                }

                if (m_StuckTimer > 1.0)
                {
                    // Stuck for 1 second. Initiate evasion.
                    m_IsEvading = true;
                    m_StuckTimer = 0.0; // Reset
                    UpdatePathfinding(m_Target.GetPosition());
                }

                if (m_IsEvading)
                {
                    // Strafe right (90 degrees) to try and slide past the obstacle
                    movementAngle = 90.0;
                    yaw += 45.0; // Angled push
                    desiredSpeed = Math.Max(2.0, desiredSpeed); // Force movement speed while evading
                }
            }
            else
            {
                // If the bot intends to be stopped, it is not stuck
                m_StuckTimer = 0.0;
                m_IsEvading = false;
            }

            m_LastPos = m_Bot.GetPosition();
            m_Stamina = Math.Clamp(m_Stamina, 0.0, 100.0);

            m_Bot.SetOrientation(Vector(yaw, 0, 0));

            // Handle Doors every 0.5s
            m_DoorCheckTimer += 0.1;
            if (m_DoorCheckTimer > 0.5)
            {
                CheckAndOpenDoors();
                m_DoorCheckTimer = 0.0;
            }

            inputController.OverrideMovementSpeed(true, desiredSpeed);
            inputController.OverrideMovementAngle(true, movementAngle);
        }
        else
        {
            m_LastPos = m_Bot.GetPosition();
            m_StuckTimer = 0.0;
            m_IsEvading = false;
            // No path, turn to target and wait
            vector fallbackDir = (m_Target.GetPosition() - m_Bot.GetPosition()).Normalized();
            vector fallbackAngles = fallbackDir.VectorToAngles();
            m_Bot.SetOrientation(Vector(fallbackAngles[0], 0, 0));
            inputController.OverrideMovementSpeed(true, 0.0);
            m_Stamina += 2.0; // Fast regen when standing still
            m_Stamina = Math.Clamp(m_Stamina, 0.0, 100.0);
        }
    }

    private void CheckAndOpenDoors()
    {
        vector start = m_Bot.GetPosition();
        start[1] = start[1] + 1.0; // Chest height
        vector forward = m_Bot.GetDirection();
        vector end = start + (forward * 2.0); // Increased distance to 2m

        vector contactPos;
        vector contactDir;
        int contactComponent;

        // Cast ray to find buildings using ObjIntersectView (better for interactables like doors)
        set<Object> hitObjects = new set<Object>;
        bool hit = DayZPhysics.RaycastRV(start, end, contactPos, contactDir, contactComponent, hitObjects, null, m_Bot, false, false, ObjIntersectView);

        if (hit && hitObjects.Count() > 0)
        {
            Object obj = hitObjects[0];
            Building building = Building.Cast(obj);

            if (building)
            {
                // Find the door index from the component string
                string compName = building.GetActionComponentName(contactComponent);
                string doorName = compName;
                doorName.ToLower();

                if (doorName.Contains("door"))
                {
                    int doorIndex = building.GetDoorIndex(contactComponent);
                    if (doorIndex != -1)
                    {
                        if (!building.IsDoorOpen(doorIndex))
                        {
                            if (!building.IsDoorLocked(doorIndex))
                            {
                                building.OpenDoor(doorIndex);
                                m_Bot.SendDebugChat(string.Format("[MoraleAI] Opened door index %1 in %2", doorIndex, building.GetType()));
                            }
                            else
                            {
                                m_Bot.SendDebugChat(string.Format("[MoraleAI] Found locked door index %1 in %2 (cannot breach yet)", doorIndex, building.GetType()));
                            }
                        }
                    }
                }
            }
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
