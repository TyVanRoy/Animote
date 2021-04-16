using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ConnectedUI : MonoBehaviour
{
    public Text connectionStatus, info;
    public SettingsPanel settings;
    public AnimationsPanel animations;
    public GameObject hcalPanel;

    public enum Status{ 
        ReadySet,
        Animating,
        Idle,
        Replay,
        Export,
        HCal_init,
        HCal_working,
        HCal_ready
    }

    void Start()
    {
    }

    void Update()
    {
        
    }

    public void SetStatus(Status s)
    {
        switch (s)
        {
            case Status.ReadySet:
                info.text = "Ready... Release the button to start animating!";
                info.gameObject.SetActive(true);
                animations.gameObject.SetActive(false);
                settings.gameObject.SetActive(false);
                hcalPanel.SetActive(false);
                break;
            case Status.Animating:
                info.text = "Animating... press the button to stop.";
                info.gameObject.SetActive(true);
                animations.gameObject.SetActive(false);
                settings.gameObject.SetActive(false);
                hcalPanel.SetActive(false);
                break;
            case Status.Idle:
                info.text = "Press and hold the remote button to begin.";
                info.gameObject.SetActive(true);
                animations.gameObject.SetActive(true);
                settings.gameObject.SetActive(true);
                hcalPanel.SetActive(false);
                break;
            case Status.Replay:
                animations.gameObject.SetActive(false);
                settings.gameObject.SetActive(false);
                info.gameObject.SetActive(false);
                hcalPanel.SetActive(false);
                break;
            case Status.Export:
                info.text = "Exporting...";
                animations.gameObject.SetActive(false);
                settings.gameObject.SetActive(false);
                info.gameObject.SetActive(true);
                hcalPanel.SetActive(false);
                break;
            case Status.HCal_init:
                info.text = "Follow the inctructions." +
                            "\nThen press the remote button when ready.";
                animations.gameObject.SetActive(false);
                settings.gameObject.SetActive(false);
                info.gameObject.SetActive(true);
                hcalPanel.SetActive(true);
                break;
            case Status.HCal_working:
                info.text = "Working..." +
                            "\nMake sure the instructions are still being followed...";
                animations.gameObject.SetActive(false);
                settings.gameObject.SetActive(false);
                info.gameObject.SetActive(true);
                hcalPanel.SetActive(true);
                break;
            case Status.HCal_ready:
                info.text = "Calibration ready!" +
                            "\nPress the button to finish.";
                animations.gameObject.SetActive(false);
                settings.gameObject.SetActive(false);
                info.gameObject.SetActive(true);
                hcalPanel.SetActive(true);
                break;
        }
    }

    public void Disconnect()
    {
        connectionStatus.text = "Not Connected";
        connectionStatus.color = Color.red;
    }

    public void Connect()
    {
        connectionStatus.text = "Connected!";
        connectionStatus.color = Color.green;
    }


}
