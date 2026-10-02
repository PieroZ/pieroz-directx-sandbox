using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class NpcManager : MonoBehaviour
{
    public EnemyController [] NpcPrefab;
    private List<EnemyController> objectPool = new();
    private HashSet<Vector2Int> usedSpawners = new();
    private Camera mainCamera;
    private Dictionary<Vector3Int, List<EnemyController>> dormantEnemyControllers = new();
    static private List<string> prefix = new List<string>{"Elder ", "Grand ", "Ancient ", "Rotten ", "Hollow ", "Mythical ", "Oppresive ", "Legendary ", "Elite ", ""};
    static private List<string> type = new List<string> { "Pumpkin", "Stalker", "Zombie", "Veggie", "Destroyer", "Slayer", "Murderer", "Assassin" };
    public static NpcManager Instance { get; private set; }

    void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
        }
        else
        {
            Destroy(gameObject);
            return;
        }
    }

    public IEnumerator UpdateActiveInHierarchy()
    {
        while (true)
        {
            for (int i = 0; i < objectPool.Count; i++)
            {
                EnemyController enemyController = objectPool[i];
                if (((Vector2)enemyController.transform.position - (Vector2)mainCamera.transform.position).magnitude > 10)
                {
                    objectPool.RemoveAt(i);
                    Vector3Int tilePosition = WorldMapManager.Instance.WorldToTilemapCoord(enemyController.transform.position);
                    tilePosition.z = 0;
                    if (!dormantEnemyControllers.ContainsKey(tilePosition))
                    {
                        dormantEnemyControllers.Add(tilePosition, new());
                    }
                    dormantEnemyControllers[tilePosition].Add(enemyController);
                    enemyController.gameObject.SetActive(false);
                    i--; // To compensate for the removed element's 
                }
            }
            yield return new WaitForSeconds(5);
        }
    }

    private void Start()
    {
        mainCamera = Camera.main;
        StartCoroutine(UpdateActiveInHierarchy());
    }

    private void OnDestroy()
    {
        StopAllCoroutines();
    }

    public void SpawnNpc(Vector2Int spawnerPosition, Vector3 position)
    {
        if (usedSpawners.Contains(spawnerPosition))
        {
            return;
        }
        usedSpawners.Add(spawnerPosition);
        EnemyController enemyController = Instantiate(NpcPrefab[Random.Range(0, NpcPrefab.Length)], position, Quaternion.identity, transform);
        Damageable damageable = enemyController.GetComponent<Damageable>();

        damageable.damageableName = prefix[Random.Range(0, prefix.Count)] + enemyController.monsterTypeName;// type[Random.Range(0, type.Count)];
        objectPool.Add(enemyController);
    }

    internal void ActivateDormantNpcs(Vector3Int tilePosition)
    {
        tilePosition.z = 0;
        if (dormantEnemyControllers.ContainsKey(tilePosition))
        {
            foreach (EnemyController enemyController in dormantEnemyControllers[tilePosition])
            {
                if (enemyController != null)
                {
                    enemyController.gameObject.SetActive(true);
                    objectPool.Add(enemyController);
                }
            }
            dormantEnemyControllers.Remove(tilePosition);
        }
    }
}
