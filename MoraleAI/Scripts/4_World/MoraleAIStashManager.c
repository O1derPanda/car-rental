class MoraleAIStashManager
{
    static void GenerateStash(vector centerPos, PlayerBase player)
    {
        float radiusMin = MoraleAIConfig.Get().StashSpawnRadiusMin;
        float radiusMax = MoraleAIConfig.Get().StashSpawnRadiusMax;

        vector spawnPos = FindSafeSpawnPosition(centerPos, radiusMin, radiusMax);

        if (spawnPos != vector.Zero)
        {
            // Spawn a container that natively holds items (MediumTent folded item causes a fatal inventory crash)
            EntityAI stash = EntityAI.Cast(GetGame().CreateObject("SeaChest", spawnPos));
            if (stash)
            {
                // Spawn loot inside
                stash.GetInventory().CreateInInventory("M4A1");
                stash.GetInventory().CreateInInventory("AmmoBox_556x45_20Rnd");
                stash.GetInventory().CreateInInventory("BakedBeansCan");
                stash.GetInventory().CreateInInventory("Canteen");

                // Send RPC to client to draw a 3D Waypoint
                if (player && player.GetIdentity())
                {
                    ScriptRPC rpc = new ScriptRPC();
                    rpc.Write(spawnPos);
                    rpc.Send(player, MoraleAIRPC.RECEIVE_STASH_WAYPOINT, true, player.GetIdentity());
                }
            }
        }
        else
        {
            Print("[MoraleAI] Failed to find a safe location for the stash!");
        }
    }

    static vector FindSafeSpawnPosition(vector center, float minRadius, float maxRadius)
    {
        vector resultPos = vector.Zero;
        int maxAttempts = 20;

        for (int i = 0; i < maxAttempts; i++)
        {
            float angle = Math.RandomFloat(0, Math.PI2);
            float distance = Math.RandomFloat(minRadius, maxRadius);

            float x = center[0] + (Math.Cos(angle) * distance);
            float z = center[2] + (Math.Sin(angle) * distance);

            float y = GetGame().SurfaceY(x, z);
            vector testPos = "0 0 0";
            testPos[0] = x;
            testPos[1] = y;
            testPos[2] = z;

            // Simple check: Ensure we are not in water
            if (GetGame().SurfaceIsSea(x, z) || GetGame().SurfaceIsPond(x, z))
                continue;

            // Object collision check
            array<Object> objects = new array<Object>;
            array<CargoBase> proxyCargos = new array<CargoBase>;
            GetGame().GetObjectsAtPosition(testPos, 3.0, objects, proxyCargos); // Check 3m radius for existing objects

            bool isColliding = false;
            foreach (Object obj : objects)
            {
                if (obj.IsBuilding() || obj.IsTree() || obj.IsRock())
                {
                    isColliding = true;
                    break;
                }
            }

            if (!isColliding)
            {
                resultPos = testPos;
                break;
            }
        }

        return resultPos;
    }
}
