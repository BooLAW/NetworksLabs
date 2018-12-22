using UnityEngine;
using UnityEngine.Networking;

using System.Collections;
using System.Collections.Generic;

public class Pumpkins_respawn : NetworkBehaviour
{
    public float[] x_offset;
    public float duration_spawn;
    public uint max_pumpkins;

    private float next_spawn;

    [Command]
    public void CmdAddPumpkin(Vector3 pos)
    {
        NetworkManager.singleton.GetComponent<CustomNetworkManager>().AddTemporaryObject(4, pos);
    }

    // Use this for initialization
    void Start ()
    {
        next_spawn = duration_spawn;
    }
	
	// Update is called once per frame
	void Update ()
    {
        next_spawn -= Time.deltaTime;

        if (next_spawn <= 0)
        {
            int num_pumpkins = (int)Random.Range(1, max_pumpkins + 1);

            for (uint i = 0; i < num_pumpkins; i++)
            {
                // Clone enemy
                float selected_offset = x_offset[Random.Range(0, x_offset.Length)];
                CmdAddPumpkin(new Vector3(transform.position.x + selected_offset, transform.position.y, transform.position.z));
            }
            
            next_spawn = duration_spawn;
        }
    }
}

