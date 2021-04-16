using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class FocalManager : MonoBehaviour
{
    public float zoomSpeed = .3f;
    public Camera cam;
    public Transform focalPoint;

    [SerializeField] private MouseLook m_MouseLook;

    // Start is called before the first frame update
    void Start()
    {
        m_MouseLook = new MouseLook();
        m_MouseLook.Init(focalPoint, cam.transform);
    }

    // Update is called once per frame
    void Update()
    {
        float zoom = Input.GetAxis("Mouse ScrollWheel");
        
        if (zoom != 0)
        {
            Vector3 dif = Vector3.Normalize(cam.transform.position - focalPoint.position) * zoomSpeed * -zoom;
            this.transform.position += dif;
        }

        if(Input.GetMouseButton(0))
            m_MouseLook.LookRotation(focalPoint, cam.transform);
    }
}
