using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class PauseMenuUI : MonoBehaviour
{
    public void NewGame()
    {
        PlayerManager playerManager = FindFirstObjectByType(typeof(PlayerManager)) as PlayerManager;
        if (playerManager != null)
        {
            Destroy(playerManager.gameObject);
        }
        DataPersistenceManager.Instance.NewGame();
        SceneManager.LoadScene("MuseumScene");
    }

    public void LoadGame()
    {
        PlayerManager playerManager = FindFirstObjectByType(typeof(PlayerManager)) as PlayerManager;
        if (playerManager != null)
        {
            Destroy(playerManager.gameObject);
        }
        DataPersistenceManager.Instance.LoadGame();
        SceneManager.LoadScene("MuseumScene");
    }

    public void SaveAndExit()
    {
        DataPersistenceManager.Instance.SaveGame();
        PlayerManager playerManager = FindFirstObjectByType(typeof(PlayerManager)) as PlayerManager;
        if (playerManager != null)
        {
            Destroy(playerManager.gameObject);
        }
        SceneManager.LoadScene("MainMenu");
    }
}
