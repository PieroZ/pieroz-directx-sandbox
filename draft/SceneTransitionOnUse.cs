using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneTransitionOnUse : MonoBehaviour, IInteractable
{
    bool sceneTransitionInProgress = false;
    public string targetScene = "MuseumScene";

    public bool IsInUse()
    {
        return false;
    }

    public void Use()
    {
        if (!sceneTransitionInProgress)
        {
            sceneTransitionInProgress = true;
            PlayerManager.Instance.SetSceneTransitionPoint(transform.position);
            SceneManager.LoadScene(targetScene);
        }
    }

    public void Unuse()
    {
    }
}
