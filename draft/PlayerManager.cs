using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.SceneManagement;

public class PlayerManager : MonoBehaviour
{
    public static PlayerManager Instance { get; private set; }
    public PlayerController playerControllerPrefab;
    private PlayerController playerController;
    private CharacterHealth playerHealth = null;
    public Transform playerSpawnPoint;
    private Vector3 playerSpawnPointByMap;
    public Dictionary<string, Vector3> lastScenePositions = new();
    private Vector3 SceneTransitionPoint = Vector3.zero;
    private string lastScene = "";

    internal void SetSceneTransitionPoint(Vector3 position)
    {
        SceneTransitionPoint = position;
    }

    private void OnDestroy()
    {
        if (playerController != null)
        {
            Destroy(playerController.gameObject);
        }
        if (playerHealth != null)
        {
            playerHealth.OnDied -= OnDied;
        }
        SceneManager.activeSceneChanged -= OnSceneChanged;
    }

    private void OnSceneChanged(Scene a, Scene b)
    {
        playerSpawnPoint = GameObject.FindGameObjectWithTag("PlayerSpawnPoint")?.transform;
        if (lastScenePositions.ContainsKey(b.name))
        {
            playerSpawnPoint.position = lastScenePositions[b.name];
        }
        else if (WorldMapManager.Instance != null)
        {
            playerSpawnPointByMap = WorldMapManager.Instance.TilemapCoordToWorld(WorldMapManager.Instance.GetEntranceLocation());
            playerSpawnPointByMap.z = playerSpawnPoint.position.z;
            playerSpawnPoint.position = playerSpawnPointByMap;
        }
        PersistentObject[] persistentObjects = playerController.GetComponentsInChildren<PersistentObject>();
        foreach (PersistentObject persistentObject in persistentObjects)
        {
            persistentObject.SaveData();
        }
        PlayerController newPlayerController = Instantiate(playerController, playerSpawnPoint.position, Quaternion.identity);
        newPlayerController.gameObject.SetActive(true);
        newPlayerController.name = "Player-OnSceneChanged";
        if (lastScene != "" && lastScene != "DesertSceneInterior" && lastScene != "DesertSceneTargetInterior")
        {
            lastScenePositions[lastScene] = playerController.gameObject.transform.position;
        }
        playerHealth.OnDied -= OnDied;
        Destroy(playerController.gameObject);
        playerController = newPlayerController;
        if (playerController.TryGetComponent(out playerHealth))
        {
            playerHealth.OnDied += OnDied;
        }
        DontDestroyOnLoad(playerController.gameObject);
        if (WorldMapManager.Instance != null)
        {
            Vector3 lastScenePosition = Vector3.zero;
            if (lastScenePositions.ContainsKey(lastScene))
            {
                lastScenePosition = lastScenePositions[lastScene];
            }
            WorldMapManager.Instance.UpdateWorldOffsetByPointOfTransition(SceneTransitionPoint);
            WorldMapManager.Instance.GenerateExit(playerSpawnPoint.position, lastScene);
        }
        lastScene = SceneManager.GetActiveScene().name;
        if (lastScene != "" && lastScene != "DesertSceneInterior" && lastScene != "DesertSceneTargetInterior" && lastScene != "DarkForestScene" && lastScene != "FrostSceneInterior")
        {
            if (playerController.TryGetComponent(out SortingGroup sortingGroup))
            {
                sortingGroup.sortingLayerName = "Background";
            }
            if (playerController.TryGetComponent(out Light2D light2D))
            {
                light2D.enabled = false;
            }
        }
        else
        {
            if (playerController.TryGetComponent(out SortingGroup sortingGroup))
            {
                sortingGroup.sortingLayerName = "ForegroundLand";
            }
            if (playerController.TryGetComponent(out Light2D light2D))
            {
                light2D.enabled = true;
            }
        }
    }

    void OnDied()
    {
        lastScenePositions.Clear();
        lastScene = "";
        SceneManager.LoadScene("MuseumScene");
    }

    void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }
        else
        {
            Destroy(gameObject);
            return;
        }
    }

    private void Start()
    {
        playerController = GetPlayerController();
        playerController.name = "Player-Awake";
        if (playerController.TryGetComponent(out playerHealth))
        {
            playerHealth.OnDied += OnDied;
        }
        lastScene = SceneManager.GetActiveScene().name;
        if (lastScene != "" && lastScene != "DesertSceneInterior" && lastScene != "DesertSceneTargetInterior" && lastScene != "DarkForestScene" && lastScene != "FrostSceneInterior")
        {
            if (playerController.TryGetComponent(out SortingGroup sortingGroup))
            {
                sortingGroup.sortingLayerName = "Background";
            }
            if (playerController.TryGetComponent(out Light2D light2D))
            {
                light2D.enabled = false;
            }
        }
        else
        {
            if (playerController.TryGetComponent(out SortingGroup sortingGroup))
            {
                sortingGroup.sortingLayerName = "ForegroundLand";
            }
            if (playerController.TryGetComponent(out Light2D light2D))
            {
                light2D.enabled = true;
            }
        }
        DontDestroyOnLoad(playerController.gameObject);

        SceneManager.activeSceneChanged += OnSceneChanged;
        if (WorldMapManager.Instance != null)
        {
            WorldMapManager.Instance.GenerateExit(playerSpawnPoint.position, lastScene);
        }
    }

    public bool IsPlayerControllerAvailable()
    {
        return playerController != null;
    }

    public PlayerController GetPlayerController() {
        if (playerController == null)
        {
            if (playerControllerPrefab == null)
            {
                return null;
            }
            playerSpawnPoint = GameObject.FindGameObjectWithTag("PlayerSpawnPoint")?.transform;
            if (playerSpawnPoint == null)
            {
                return null;
            }
            if (WorldMapManager.Instance != null)
            {
                playerSpawnPointByMap = WorldMapManager.Instance.TilemapCoordToWorld(WorldMapManager.Instance.GetEntranceLocation());
                playerSpawnPointByMap.z = playerSpawnPoint.position.z;
                playerSpawnPoint.position = playerSpawnPointByMap;
            }
            playerController = Instantiate(playerControllerPrefab, playerSpawnPoint.position, Quaternion.identity);
            playerController.name = "Player-GetPlayerController";
            DontDestroyOnLoad(playerController.gameObject);
        }
        return playerController;
    }

    public Inventory GetPlayerInventoryWithTag(string tag)
    {
        foreach (Inventory inventory in GetPlayerController().GetComponentsInChildren<Inventory>())
        {
            if (inventory.inventoryTag == tag)
            {
                return inventory;
            }
        }
        return null;
    }
}
