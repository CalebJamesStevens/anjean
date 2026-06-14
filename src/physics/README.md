From https://media.steampowered.com/apps/valve/2015/DirkGregorius_Contacts.pdf
Let’s quickly look at the anatomy of a physics tick:
- We use the broadphase to detect pairs of shape proxies that can potentially be in
contact
- E.g. in an AABB tree we detect all overlapping AABB pairs
- In the narrowphase we then need to test if the actual collision shapes are touching
- If the shapes are touching we create contact information between the two shapes
and send this to the solver
- Finally the solver then advances the rigid bodies and uses the provided contact
information to prevent penetration and to simulate friction