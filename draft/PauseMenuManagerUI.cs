using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PauseMenuManagerUI : MonoBehaviour
{
    public PauseMenuUI pauseMenuUI;
    public static bool gamePaused = false;
    // Start is called before the first frame update
    void Start()
    {
        pauseMenuUI.gameObject.SetActive(false);
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetKeyDown(KeyCode.Escape))
        {
            if (!gamePaused)
            {
                Time.timeScale = 0f;
                pauseMenuUI.gameObject.SetActive(true);
            }
            else
            {
                Time.timeScale = 1f;
                pauseMenuUI.gameObject.SetActive(false);
            }
            gamePaused = !gamePaused;
        }
    }

    private void OnDisable()
    {
        gamePaused = false;
        Time.timeScale = 1f;
    }

    private void OnDestroy()
    {
        gamePaused = false;
        Time.timeScale = 1f;
    }
}
