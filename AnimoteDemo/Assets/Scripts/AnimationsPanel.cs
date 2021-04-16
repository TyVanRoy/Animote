using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AnimationsPanel : MonoBehaviour
{
    public Transform listingParent;
    public AnimationListing listingComponent;
    private ArrayList listings;

    // Start is called before the first frame update
    void Start()
    {
        listings = new ArrayList();
    }

    // Update is called once per frame
    void Update()
    {
    }

    public void AddAnimation(AnimoteAnimation animation, string name)
    {
        AnimationListing listing = Instantiate(listingComponent, listingParent);
        listing.Init(animation, name, AnimoteClient.instance.transform);
    }
}