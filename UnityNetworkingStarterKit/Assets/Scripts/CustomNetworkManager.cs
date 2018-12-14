using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class MsgTypes
{
    public const short PlayerPrefabSelect = MsgType.Highest + 1;
    public class PlayerPrefabMsg : MessageBase
    {
        public short controllerId;
        public short prefabIndex;
    }
}

public class CustomNetworkManager : NetworkManager {

    public short playerPrefabIndex;

    public int selectedGridIndex = 0;
    public string[] playerNames = new string[] { "Boy", "Girl", "Robot" };

    // Interactive player prefab selection /////////////////////////////////////////
    private void OnGUI()
    {
        if (!isNetworkActive)
        {
            selectedGridIndex = GUI.SelectionGrid(
            new Rect(Screen.width - 200, 10, 200, 50),
            selectedGridIndex,
            playerNames,
            3);
            playerPrefabIndex = (short)(selectedGridIndex + 1);
        }
    }

    public override void OnStartServer()
    {
        NetworkServer.RegisterHandler(MsgTypes.PlayerPrefabSelect, OnPrefabResponse);
        base.OnStartServer();
    }

    public override void OnClientConnect(NetworkConnection conn)
    {
        client.RegisterHandler(MsgTypes.PlayerPrefabSelect, OnPrefabRequest);
        base.OnClientConnect(conn);
    }

    public override void OnServerAddPlayer(NetworkConnection conn, short playerControllerId)
    {
        MsgTypes.PlayerPrefabMsg msg = new MsgTypes.PlayerPrefabMsg();
        msg.controllerId = playerControllerId;
        NetworkServer.SendToClient(conn.connectionId, MsgTypes.PlayerPrefabSelect, msg);
    }

    private void OnPrefabRequest(NetworkMessage netMsg)
    {
        MsgTypes.PlayerPrefabMsg msg = netMsg.ReadMessage<MsgTypes.PlayerPrefabMsg>();
        msg.prefabIndex = playerPrefabIndex;
        client.Send(MsgTypes.PlayerPrefabSelect, msg);
    }

    private void OnPrefabResponse(NetworkMessage netMsg)
    {
        MsgTypes.PlayerPrefabMsg msg = netMsg.ReadMessage<MsgTypes.PlayerPrefabMsg>();
        playerPrefab = spawnPrefabs[msg.prefabIndex];
        base.OnServerAddPlayer(netMsg.conn, msg.controllerId);
    }

    public void ChangePlayerPrefab(PlayerController currentPlayer, int prefabIndex)
    {
        GameObject newPlayer = Instantiate(spawnPrefabs[prefabIndex],
        currentPlayer.gameObject.transform.position,
        currentPlayer.gameObject.transform.rotation);
        playerPrefab = spawnPrefabs[prefabIndex];
        NetworkServer.Destroy(currentPlayer.gameObject);
        NetworkServer.ReplacePlayerForConnection(
       currentPlayer.connectionToClient, newPlayer, 0);
    }

    public void AddObject(int objIndex, Vector3 pos)
    {
        GameObject newObject = Instantiate<GameObject>(spawnPrefabs[objIndex], pos, Quaternion.identity);

        NetworkServer.Spawn(newObject);
    }

}
