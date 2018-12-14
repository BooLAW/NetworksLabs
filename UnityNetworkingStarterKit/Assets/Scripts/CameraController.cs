using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraController : MonoBehaviour {

    // We make it static so that it can be changed globally
    static public Transform player;

	// Use this for initialization
	void Start () {
	}
	
	// Update is called once per frame
	void Update ()
    {
        if (player)
        {
            transform.SetPositionAndRotation(player.position + new Vector3(0.0f, 5.0f, -5.0f), Quaternion.identity);
            transform.LookAt(player.position + new Vector3(0.0f, 2.0f, 0.0f), Vector3.up);
        }
	}
}
