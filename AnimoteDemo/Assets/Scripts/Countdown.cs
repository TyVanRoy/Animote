using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class Countdown : MonoBehaviour
{
    public int seconds = 3;
    public GameObject rep;
    public Text text;
    private bool counting = false;

    private int count;
    private float origin;

    // Start is called before the first frame update
    void Start()
    {
        rep.SetActive(false);
        counting = false;
    }

    // Update is called once per frame
    void Update()
    {
        if (counting)
        {
            if (count >= seconds)   // finished
            {
                rep.SetActive(false);
                counting = false;
            }
            else if (Time.time >= (origin + 1))
            {
                count++;
                origin = Time.time;

                text.text = (seconds - count).ToString();
            }
        }
    }

    public bool Done()
    {
        return !counting;
    }

    public float PercentComplete()
    {
        if (!counting)
            return -1f;
        return (float) count / (float) seconds;
    }

    public void Begin()
    {
        counting = true;
        count = 0;

        origin = Time.time;

        text.text = seconds.ToString();
        rep.SetActive(true);
    }
}
