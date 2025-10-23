# Script

## slide 1

Hello everyone, and welcome to my thesis presentation!
My name is David, and my oppontent Matias will be joining from Zoom.
My supervisor for this project is Björn Thuresson, and the examiner is Tino Weinkauf.

Today, I'm going to tell you about 
Probe Placement Strategies for Radiance Cascades with 
Screen-space Probes and World-space Intervals,
which is a technique for computing the first bounce of global illumination.

## slide 2

Today's agenda looks like this.
First I'm going to give you a brief introduction to what I've been doing,
where I'll quickly go over the problem background and theory, 
as well as the research questions I've been basing my work on.

## slide 3

First off - what is this term - Global Illumination?

It's an umbrella term, 
encapsulating a lot of effects like indirect illumination,
specular reflections, 
refraction, 
soft shadows, etc. 
Usually people try to explain it by showing two images like these:

## slide 4 & 5

Here we see only direct light being applied to the scene.
A lot of the screen is completely black, 
because no light ever arrives there.
[flip to 5]
And here, we have indirect lighting, 
as well as some specular effects down on the floor and on these leaves.
We can see a lot more detail in the arches,
and we start to feel like this polished stone floor is actually reflecting light from the sky.
You can almost feel the warmth of the sun in the sky.
[flip back & forth a bit]

Actually, [back to 4] for our purposes today,
we have global illumination here already.
We are actually evaluating lighting from every direction, 
every single point in the sky here, (because the sky is the only light source)
notice how the top of the wall is a bit orange-ish, while the floor isn't.
That's beecause the sun is low in the sky, at dawn.
The sky is essentially one big area light.
We're going to limit our understanding of global illumination to 
just having accurate illumination and shadows for an arbitrary amount of arbitrarily shaped area lihghts.

This is what we're going to be trying to approximate, and now,
we're going to have to go into the maths.

## slide 6

This is the rendering equation, introduced by Kajiya in 1986.
Essentially it says that the light coming from a point P in a direction Omega is equal to
the light emitted from P in that direction, which is usually zero unless it's a light source, 
plus an integral over all of the light arriving at that point, 
scaled by the BRDF (or BSDF) denoted f, 
which describes how the material at P interacts with light.

Note that this is a recursive function, as L-in is the result of some L-out at another point. 
This means, that indirect illumination like we saw in the second picture is attained by simply 
computing more recursion steps of this function. 

In that first picture we also saw that there were no specular effects, 
which just means that our BRDF f is perfectly diffuse.

## slide 7

Because we're only interested in diffuse lighting, we can simplify a lot.
This is the so called fluence rate at a point p, 
which is a measure of how much light passes through said point.
We are assuming that if we evaluate this at a surface, and scale by the surface normal and the diffuse BRDF, 
we get roughly the irradiance we're looking for.

So how do we compute this? Enter Radiance cascades.

## slide 8

What is this?

## slide 9

Alexander Sannikov came up with radiance cascades while working on global illumination for 
the game Path of Exile 2.
A paper was "published" but not officially in any journal.
It doesn't go into much detail beyond the overall idea in 2D...
Some ideas and a proof of concept for 3D are presented. (and we know it works because it's in PoE2)
Two other preprints exist, but mostly focus on enhancements for the 2D version.
Which is why I'm looking at its applicability to 3D.

## slide 10

Back to mathematics,
that L we saw in the Fluence rate function, 
is defined like this. 
This is basically a raytracing operator,
we evaluate every point along a ray from P in direction Omega
and check if it's hit a surface, and if not we add the light (L-s) at that point.
This also works with a non-binary variant of T, but we're ignoring transparency for simplicity.

## slide 11

Now consider this situation.
We have a light source and a wall blocking said light,
resulting in a penumbra. 
Looking at the angles the light source takes up,
from the perspective of a point at a given distance, see alpha and beta,
we note that the angle is inversely proportional to the distance.
So, the further we are from the light, the more angles we need to check to capture all details.
On the other hand, at longer distances, the light also affects greater areas, 
so we don't need as high a spatial frequency to probe the far distances.


## slide 12

So, our idea is to split near- and far lighting, 
and evalute fewer angles at close distances to each point.
That L we were using for the fluence rate can be redefined as the conditional sum of the near and far L, 
where the far part is only used if the near part didn't hit any geometry.
We call this process merging.

## slide 13

We end up with something like this. 
On the left we have a visualization of how light at the origin is computed in the green, turquoise and blue ranges,
with more angles covered by the far field and fewer at shorter distances.
On the right, we see how we divide probes across the screen.
A lot of short-range probes, and fewer long range probes. 
We then interpolate between the longer range probes when merging into the short range ones.

## slide 14

So when computing this green short range interval, we average these two turquoise ones, 
which in turn average from these 4 blue intervals.
In practice, we get what's on the right, 
where the green short range probe interpolates between the averages of four turquoise probes 
based on its relative position to them, and the turquoise probes gather from the blue ones. 
In this case, only one C-2 probe could fit on the screen, so all of them just use this one.
You may also notice that the number of rays cast from each probe matches the number of neighboring pixels 
(or would match with a 4x branching factor),
which means the memory required to store each cascade level is constant.
These are of course not to scale, and you would want to go up to a 4x branching factor in 2D still, here we have 2.

## slide 15

Now imagine instead of the 2D circles you've been looking at, they're 3D spheres.
We map the spheres to the 2D pixels using Clarberg/octahedral mapping, introduced in 2008 by Clarberg.

How we place the probes in 3D is also important. 
We could imagine expanding the 2D grid to a 3D one in world-space, 
but this quickly becomes prohibitively expensive as the scene grows.

Instead we place the probes in screen-space by projecting a 2D grid (or grids) onto the geometry visible from the camera.

## slide 16

Sannikov presented two directions to go in from here.
SPWI - screen-space probes, world-space intervals, and
SPSI - screen-space probes, screen-space intervals.
SPWI traces the intervals we've seen in world-space which means all walls and all light sources will be able to affect every probe.
But this comes at the expense of having to ray-trace the whole scene...
SPSI can go much faster, as tracing rays in screen-space has much more potential for optimization,
but doesn't allow off-screen geometry to contribute. 

## slide 17

Because PoE2 uses SPSI, and I am a contrarian, I went with SPWI for this thesis.
No, the real reason is that optimizing screen-space raytracing wasn't in scope for this project.

## slide 18

Now we're getting into what I've actually been investigating, which is how to place the probes in screen-space.
The original RC paper suggested a bilateral interpolation scheme for merging each cascade, 
in which each probe is placed at the maximum depth in the cell it's been assigned in the screen-space grid.
Bilateral interpolation is essentially bilinear interpolation, but also weighted by the difference in depth and normal between the interpolation
points and the target probe.
Sannikov later thought to place a second layer of probes at the maximum depth in each cell, 
and do a form of non-uniform trilinear interpolation. 
This was never published, but he has spoken a bit about it in the radiance cascades discord server.
The circled green C-i probes are merged with data from the circled blue C-i+1 probes.
These images are top-down slices with the camera at the bottom, so the y-axis can't be seen, 
imagine there are blue and green probes in the third dimension as well.

## slide 19

These are the Research questions I used to guide my work.
[read aloud]

## slide 20

So what exactly did I do?
I implemented SPWI using AMDs Capsaicin framework, 
both the Bilateral and Min+Max varieties, using hardware ray-tracing for the world-space intervals.
This framework provided a reference path tracer, and ways to track frame time so all the comparisons can be done
with these as a basis.

## slide 21

I then evaluated both Bilateral and Min+Max at 3 probe grid resolutions in 2 scenes,
tracking the frame time and the SSIM and NVIDIA FLIP scores comparing to the reference path tracer.

These metrics are common in this field, see these references.

## slide 22

Now we're getting into the results, and we're gonna start with the image comparisons.
These are captures from the Cornell Box scene, with bilateral on the left and min+max on the right.
It's using the highest probe grid resolution, which means one probe per pixel at C-0,
and 5 cascades.

You'll notice that there are some jagged artifacts, particularly in the Bilateral render,
and that both implementations fail to shade the close side of the big block, 
as well as the upper edge of the small block.
There's also some regular patterns on the walls where interpolation fails,
and ringing near the cascade edges, which is a common problem with RC.

## slide 23

Now the sponza scene, with 1 probe per pixel at C-0, 5 cascades.
Significantly more orange, which if you haven't seen flip error maps before, is bad.
This is either because of an error in how environment maps are treated in my implementation, 
we can see that the brightness of the image isn't quite right.
or because the pure-diffuse approximation is too aggressive to be used with textures, 
we can see that a lot of detail missing on this left pillar.
Contact shadows are also missing near the pillar bases, which can be improved with something called C--1 gathering,
which ill come back to later.

## slide 24

Here are the numerical flip error values for both scenes at all resolutions. 
We can see that min+max is consistently better, and that higher resolution means better results,
but not by a whole lot.
And the significantly higher errors in the sponza scene are due to what I just mentioned.

## slide 25

Here are the SSIM scores for both scenes at all resolutions.
They match the flip values (since ssim is higher-better) 
but a lot of minute differences in the sponza scene aren't captured.
This really just means flip is a more reliable metric than ssim for these kinds of measurements.

## slide 26

Here's a comparison of bilateral and min+max in the sponza scene at the lowest probe grid resolution
(apologies for the low image resolution, don't know what happened when i exported this version)
at 1 probe per 4x4 pixels.
As can be seen in the circled areas,
the bilateral version leaks light across cascade levels a lot more than min+max.
And yet, as we saw in the graphs, these blatant differences aren't that important to the numbers gotten from flip or ssim.

## slide 27

Here are the frame times for rendering both scenes.
As you can see, large numbers of rays traced means severe dependence on scene complexity, 
with the Sponza frame times scaling nearly twice as fast as the resolution increases.
For reference, to achieve interactive 60fps, you want your frame time to be below 16.67ms, 
and this is including every other computation you may need to do such as physics and game logic in a game.
Which means that we'd be forced to use the lowest resolution if we wanted to run in real-time on the 2070 machine.
On another 4080 super machine,
the higher resolutions were also acceptable,
but I didn't do any thorough data gathering on that one.

Min+Max is up to 17% slower than bilateral in all cases, which doesn't make the minor 0-6% image quality improvements look very good.

## slide 28

Now I'm taking my own advice and showing you some video, 
because this highlights some of the artifacts that image similarity metrics can't capture.
These are at the highest probe grid resolution, 5 cascades, so the frame rate is very low,
but what's important here are the big flickering artifacts that show up in bilateral on the left but not as much on the right with min+max.

So despite min+max being significantly slower, and not much better in terms of flip and ssim, 
not having these artifacts is a big bonus.

## slide 29

What can we conclude from this?
Back to the research questions.

Min+max is about 2-17% slower, and computation times on the 2070 range from 9 to 160ms per frame.

In image comparisons, Min+max is about 0-6% better than bilateral.

Min+Max scales better in image quality as resolution increases but worse than bilateral in frame time.

The most important tradeoffs are the spatial artifacts and frame time. 
Missing out on the horrible artifacts is a good reason to consider the 17% slower min+max approach.

## slide 30

Overall, min+max is better.
Despite it being slower and using twice the memory due to the double probe buffers needed, 
the image quality is better not only according to flip and ssim, 
but also it doesn't have the horrible artifacts.

## slide 31

We can also conclude that the image quality metrics used don't capture some of the most important details,
and, I didn't mention this yet, but these results were all based on LDR images. 
I also ran FLIP and SSIM on HDR images, where the results were not as consistent, suggesting that these metrics may not handle HDR images very well. 

The frame times are too long to be used in real-time on this hardware, so a lot more optimization is needed.

There's an error either in how environment maps are handled or in the pure diffuse assumption we made when using the fluence rate.

The images don't compare very well to the reference, which may be somewhat improved with c--1 gathering.

## slide 32

So what's next?

Lots of optimizations are possible here. 
Future hardware would speed things up, but that's expensive, 
and assuming all users of software using this technique can access expensive hardware is not to be taken lightly.
Binning rays together, and separating some of the shader code into separate programs could improve performance,
and c--1 gathering, which essentially just means doing the same trace and merge step on each pixel to the point where nearby probes were placed could
help a lot.

One might also investigate in what situations it's even worth using SPWI when SPSI has been proven to be applicable 
in a real-time application, PoE2.

There's also a lot of space for investigating how to get indirect lighting into an RC pipeline,
whether it be fully screen-space or using a world-space hash like AMDs GI-1.1.

## slide 33

That's all I could remember to put in the presentation, so thank you for listening!
Now it's time for the opposition and the question segment.

