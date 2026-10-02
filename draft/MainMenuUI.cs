using UnityEngine;
using UnityEngine.SceneManagement;

public class MainMenuUI : MonoBehaviour
{
    public void NewGame()
    {
        PlayerManager playerManager = FindFirstObjectByType(typeof(PlayerManager)) as PlayerManager;
        if (playerManager != null)
        {
            Destroy(playerManager.gameObject);
        }
        DataPersistenceManager.Instance.NewGame();
        StoryManager.Instance.TriggerStory("NewGameStory");
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

    public void Exit()
    {
#if UNITY_EDITOR
        UnityEditor.EditorApplication.ExitPlaymode();
#else
        Application.Quit();
#endif
    }
}
