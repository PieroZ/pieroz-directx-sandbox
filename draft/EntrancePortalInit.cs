using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Text.RegularExpressions;


public class EntrancePortalInit : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        string targetScene = WorldMapManager.Instance.GetExitTargetScene();
        if (targetScene != "")
        {
            GetComponent<SceneTransitionOnUse>().targetScene = targetScene;
            GetComponent<InteractableTooltipUIWrapper>().interactableName = Regex.Replace(targetScene, "Scene", string.Empty);
        }
        enabled = false;
    }
}
