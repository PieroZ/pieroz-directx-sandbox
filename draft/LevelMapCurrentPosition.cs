using UnityEngine;
using UnityEngine.UI;

public class LevelMapCurrentPosition : MonoBehaviour
{
    public int width = 256; // Width of the generated texture
    public int height = 256; // Height of the generated texture

    private int halfWidth = 128;
    private int halfHeight = 128;
    private Texture2D texture; // Reference to the generated texture
    private RawImage rawImage; // Reference to the RawImage component
    private Camera mainCamera;
    private Vector2Int lastPosition = Vector2Int.zero;
    private void Awake()
    {
        texture = new Texture2D(width, height);
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y ++)
            {
                texture.SetPixel(x, y, Color.clear);
            }
        }
        halfWidth = width / 2;
        halfHeight = height / 2;
    }
    private void Start()
    {
        rawImage = GetComponent<RawImage>(); // Get the RawImage component attached to the GameObject
        mainCamera = Camera.main;
    }

    private void Update()
    {
        DrawCurrentPosition();
    }

    private void DrawCurrentPosition()
    {
        if (WorldMapManager.Instance == null)
        {
            return;
        }

        Vector2Int currentPosition = (Vector2Int)WorldMapManager.Instance.WorldToTilemapCoord(mainCamera.transform.position);
        if (currentPosition == lastPosition)
        {
            return;
        }
        texture.SetPixel(lastPosition.x + halfWidth, lastPosition.y + halfHeight, Color.clear);
        texture.SetPixel(currentPosition.x + halfWidth, currentPosition.y + halfHeight, Color.red);
        texture.Apply();
        rawImage.texture = texture; // Assign the generated texture to the RawImage component
        lastPosition = currentPosition;
    }
}