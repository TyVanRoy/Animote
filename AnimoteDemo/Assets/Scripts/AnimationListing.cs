using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class AnimationListing : MonoBehaviour
{
    public Text titleField;
    private Transform target;
    private string name;
    private AnimoteAnimation animation;
    private Reconstruction recon;

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public void Init(AnimoteAnimation animation, string name, Transform target)
    {
        this.animation = animation;
        this.name = name;

        titleField.text = name;

        this.target = target;

        recon = AnimoteClient.instance.reconstructor.Reconstruct(animation);
    }
    
    public void PlayReconstruction()
    {
        if (recon != null)
            AnimoteClient.instance.reconstructor.Play(recon, target);
    }

    public void ExportReconstruction()
    {
        if (recon != null)
            AnimoteClient.instance.reconstructor.Export(recon, target);
    }
}
