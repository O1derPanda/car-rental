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
    private float m_EvasionTimer;
    private int m_EvasionPhase;

    // Combat Logic
    private float m_FireBurstTimer;
    private int m_ShotsToFire;
    private float m_AimTimer;

    void MoraleAIBrain(MoraleAIBotBase bot)
    {
        m_Bot = bot;
        m_Path = new array<vector>();
        m_Stamina = 100.0;
        m_IsExhausted = false;
        m_DoorCheckTimer = 0.0;
        m_StuckTimer = 0.0;
        m_EvasionTimer = 0.0;
        m_EvasionPhase = 0;
        m_FireBurstTimer = 0.0;
        m_ShotsToFire = 0;
        m_AimTimer = 0.0;
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

            // Combat range logic
            bool inCombatRange = false;

            if (distanceToTarget <= 30.0)
            {
                desiredSpeed = 0.0; // Stop to shoot
                m_Stamina += 1.0;
                inCombatRange = true;
            }
            else if (distanceToTarget > 50.0)
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

            // Handle Evasion State
            if (m_EvasionTimer > 0)
            {
                m_EvasionTimer -= 0.1;

                // Force evasive action for the duration of the timer
                if (m_EvasionPhase == 1)
                {
                    movementAngle = 90.0; // Strafe Right
                    yaw += 45.0;
                }
                else if (m_EvasionPhase == 2)
                {
                    movementAngle = -90.0; // Strafe Left
                    yaw -= 45.0;
                }
                else if (m_EvasionPhase == 3)
                {
                    movementAngle = 180.0; // Move Backwards
                    // Do not change yaw, back up straight
                }

                desiredSpeed = 2.0; // Force movement speed while evading
                m_StuckTimer = 0.0; // Keep stuck timer reset while evading

                if (m_EvasionTimer <= 0)
                {
                    // Evasion complete. Request a fresh path.
                    UpdatePathfinding(m_Target.GetPosition());
                }
            }
            else
            {
                // Handle Normal Anti-Stuck Logic (ONLY if trying to move and not already evading)
                if (desiredSpeed > 0.0)
                {
                    float distMoved = vector.Distance(m_Bot.GetPosition(), m_LastPos);
                    if (distMoved < 0.05) // Barely moved in 0.1s
                    {
                        m_StuckTimer += 0.1;
                    }
                    else
                    {
                        // Moving fine, reset timers/phases
                        m_StuckTimer = 0.0;
                        m_EvasionPhase = 0;
                    }

                    // Trigger Evasion Phases
                    if (m_StuckTimer > 3.0)
                    {
                        m_EvasionTimer = 1.0; // Evade for 1 second
                        m_EvasionPhase = 3;
                    }
                    else if (m_StuckTimer > 2.0 && m_EvasionPhase < 2)
                    {
                        m_EvasionTimer = 0.5; // Evade for 0.5 seconds
                        m_EvasionPhase = 2;
                    }
                    else if (m_StuckTimer > 1.0 && m_EvasionPhase < 1)
                    {
                        m_EvasionTimer = 0.5; // Evade for 0.5 seconds
                        m_EvasionPhase = 1;
                    }
                }
                else
                {
                    // If intending to be stopped, not stuck
                    m_StuckTimer = 0.0;
                    m_EvasionPhase = 0;
                }
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

            // Combat Engagement
            HandleCombatEngagement(inputController, inCombatRange);
        }
        else
        {
            m_LastPos = m_Bot.GetPosition();
            m_StuckTimer = 0.0;
            m_EvasionTimer = 0.0;
            m_EvasionPhase = 0;
            // No path, turn to target and wait
            vector fallbackDir = (m_Target.GetPosition() - m_Bot.GetPosition()).Normalized();
            vector fallbackAngles = fallbackDir.VectorToAngles();
            m_Bot.SetOrientation(Vector(fallbackAngles[0], 0, 0));
            inputController.OverrideMovementSpeed(true, 0.0);
            m_Stamina += 2.0; // Fast regen when standing still
            m_Stamina = Math.Clamp(m_Stamina, 0.0, 100.0);

            // Still try to shoot if fallback turning
            if (m_Target && m_Target.IsAlive())
            {
                float fallbackDist = vector.Distance(m_Bot.GetPosition(), m_Target.GetPosition());
                HandleCombatEngagement(inputController, fallbackDist <= 30.0);
            }
        }
    }

    private void HandleCombatEngagement(HumanInputController inputController, bool inCombatRange)
    {
        if (inCombatRange && m_Target && m_Target.IsAlive())
        {
            // Face target directly when shooting
            vector directDir = (m_Target.GetPosition() - m_Bot.GetPosition()).Normalized();
            vector directAngles = directDir.VectorToAngles();
            m_Bot.SetOrientation(Vector(directAngles[0], 0, 0));

            // Crouch for accuracy
            if (m_Bot.GetCommand_Move())
            {
                m_Bot.GetCommand_Move().ForceStance(DayZPlayerConstants.STANCEIDX_CROUCH);
            }

            // Override Input to aim weapon
            inputController.OverrideRaise(true, true);
            inputController.OverrideAimChangeX(true, 0.0);
            inputController.OverrideAimChangeY(true, 0.0);

            // Wait for 1 second for the raise animation to complete before firing
            m_AimTimer += 0.1;

            if (m_AimTimer > 1.0)
            {
                m_FireBurstTimer += 0.1;
                if (m_ShotsToFire > 0)
                {
                    // Force fire via Weapon FSM Event Trigger
                    EntityAI entityInHands = m_Bot.GetHumanInventory().GetEntityInHands();
                    Weapon_Base weapon;
                    if (Class.CastTo(weapon, entityInHands))
                    {
                        bool canFire = weapon.CanFire();
                        bool isChamberFull = weapon.IsChamberFull(0);
                        bool isChamberEmpty = weapon.IsChamberEmpty(0);
                        bool isJammed = weapon.IsChamberJammed(0);

                        if (canFire)
                        {
                            weapon.ProcessWeaponEvent(new WeaponEventTrigger(m_Bot));
                            m_Bot.SendDebugChat("[MoraleAI] *BANG* CanFire=TRUE, triggered shot!");
                        }
                        else
                        {
                            m_Bot.SendDebugChat(string.Format("[MoraleAI] CanFire=FALSE | Full:%1 Empty:%2 Jam:%3 | Forcing trigger anyway...", isChamberFull, isChamberEmpty, isJammed));
                            // Force trigger anyway to see if the FSM catches it
                            weapon.ProcessWeaponEvent(new WeaponEventTrigger(m_Bot));
                            m_ShotsToFire = 0; // Abort this burst
                        }
                    }
                    m_ShotsToFire--;
                }
                else
                {
                    // Start new burst every 2 seconds
                    if (m_FireBurstTimer > 2.0)
                    {
                        m_ShotsToFire = Math.RandomIntInclusive(2, 5); // Fire 2 to 5 bullets
                        m_FireBurstTimer = 0.0;
                    }
                }
            }
            else
            {
                // Prevent log spam, only send occasionally
                if (m_FireBurstTimer > 1.0)
                {
                    m_Bot.SendDebugChat("[MoraleAI] Waiting for weapon to raise... (AimTimer)");
                    m_FireBurstTimer = 0.0;
                }
                else
                {
                    m_FireBurstTimer += 0.1;
                }
            }
        }
        else
        {
            // Reset combat states
            inputController.OverrideRaise(true, false);
            m_ShotsToFire = 0;
            m_AimTimer = 0.0;

            if (m_Bot.GetCommand_Move())
            {
                m_Bot.GetCommand_Move().ForceStance(DayZPlayerConstants.STANCEIDX_ERECT);
            }
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
