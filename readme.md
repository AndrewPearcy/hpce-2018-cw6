CW6 : Overview
==============

This code-base describes a basic test-bench which is intended to
evaluate the performance of a traditional frame-based classifier
against an event-driven classifier. The overall context is that
someone is interested in identifying known patterns in video
streams, but wants to do so with very low latency (< 1 ms)
with large frame sizes (up to HD). Their choice of solution then looks like:

- Traditional image capture using a 30 FPS camera. 
  Each frame is streamed off for processing, with a minimum
  detection time of at least 70ms: 33ms to capture the frame,
  20 ms to get it off the camera, and 20 ms to perform full-frame
  object detection.

- Even-driven capture using a [spiking camera](http://rpg.ifi.uzh.ch/research_dvs.html), which produces a stream
  of events describing when individual pixels change. The event stream
  is processed progressively as the spikes are received, reducing
  the latency to less than 1ms. They know how to build custom hardware
  object detection which is able to perform object detection in a
  massively parallel way, but are limited in how many spikes per
  second it can consume.

The unknown factor is whether the degradation in image quality due to
the spike based sampling is offset by the reduction in latency of
the detection, and until this question is answered it would be too
risky to design and deliver the custom hardware solution.

This code-base is a decision support tool which is being used to answer
that question, and was thrown together by a combination of the camera
engineers and the object detection hardware engineers to mimic the
qualitative behaviour of the system - essentially:
- does the system still manage to detect objects; and
- how long does it take to detect them.

It turns out the system is quite slow (as they were hardware specialists),
so getting the results for many scenarios is becoming a bottleneck.
Bringing those two groups together was expensive, and it's not clear they
would make it much faster anyway, so the decision has been made to
bring in an external team to investigate speeding the application up as-is.

Your team has been asked to do an initial analysis of the application,
and propose how a follow-up team could deliver a much faster version of
the application, while providing information about whether it would be
worth the effort.

So the client's interest here is _not_ to deliver a fast or low-latency solution for 
object detection using multi-core or GPUs, their goal is to get this decision support
application/simulator to provide results faster and help them decide whether to
build hardware or not.

Similarly, your goal is _not_ simply to make the application faster, it is
to analyse the application and propose how to deliver a much faster system
with the investment of more person-hours. A by-product of this
analysis is likely to involve making the entire application a bit faster
using obvious techniques, as you need to convince people that you understand the
current application. Another by-product of the proposal is likely to involve
point-wise investigations into key bottlenecks you have identified, where you try
to demonstrate that you know how to solve the most important problems. There may
be lots of routine engineering involved in the proposal, but we can bring
in competent software engineers for that if the bid is accepted.

Spiking Video Streams
---------------------

A traditional frame-based camera produces complete frames at some
fixed time period ![\delta_f](https://latex.codecogs.com/svg.latex?\delta_f), so it produces a sequence of frames:

![f_0, f_1, f_2, ...](https://latex.codecogs.com/svg.latex?f_0,f_1,f_2,\\&space;\hdots)

where frame ![f_i](https://latex.codecogs.com/svg.latex?f_i) is associated with the relative time point ![i \times \delta_f](https://latex.codecogs.com/svg.latex?i\times{\delta_f}),
and each frame is a ![w \times h \times c](https://latex.codecogs.com/svg.latex?w\times{h}\times{c}) tensor (well, technically a holor).
Typically ![c=1](https://latex.codecogs.com/svg.latex?c=1) for gray-scale or ![c=3](https://latex.codecogs.com/svg.latex?c=3) for RGB or YUV.

The event based cameras being considered produce a stream of pixel events,
which describe changes to pixel intensity at specific points in time. These
events occur at a sequence of (non-strict) monotonically increasing
time points, and each event carries an update to exactly one pixel. So we
have a sequence of events:

![e_0, e_1, e_2, ...](https://latex.codecogs.com/svg.latex?e_0,e_1,e_2,\\&space;\hdots)

with each event a tuple ![e_i=(t,x,y,v)](https://latex.codecogs.com/svg.latex?e_i=(t,x,y,v)) where:
- ![t](https://latex.codecogs.com/svg.latex?t) is when the event occurred
- ![(x,y)](https://latex.codecogs.com/svg.latex?(x,y)) is the pixel co-ordinate being updated, with ![0 \leq x < w](https://latex.codecogs.com/svg.latex?0\leq{x}<w) and ![0 \leq y < h](https://latex.codecogs.com/svg.latex?0\leq{y}<h).
- ![v](https://latex.codecogs.com/svg.latex?v) is a ![c](https://latex.codecogs.com/svg.latex?c) component vector, e.g. for RGB is a 3-dimensional vector.
Time monotonicity means that we should always have
![i < j \implies t_i \leq t_j](https://latex.codecogs.com/svg.latex?i<j\implies{t_i}\leq{t_j}).

From the spike sequence we can convert back to complete frames at
each discrete event time, though it would produce an extremely
high frame-rate stream with relatively low information content
(as only one pixel changes per frame).

The relative advantages of frame-based and event-based camers comes down
to a tradeoff between:

- latency: the frame-based camera has a fixed temporal resolution of
  ![\delta_f](https://latex.codecogs.com/svg.latex?\delta_f),
  while the temporal resolution of the event-based camera depends on how quickly
  pixels are changing, and how many pixels are changing.

- bandwidth: the frame-based camera requires fixed band-width of
  ![O( w \times h \times c / \delta_f)](https://latex.codecogs.com/svg.latex?O(w\times{h}\times{c}/{\delta_f}))
  while the event-based camera bandwidth varies with over time; between time points
  ![t_a](https://latex.codecogs.com/svg.latex?t_a) and
  ![t_b](https://latex.codecogs.com/svg.latex?t_b) the bandwidth will be proportional to
  ![| { e_i : t_a < t_i < t_b } | / (t_b-t_a)](https://latex.codecogs.com/svg.latex?|e_i:t_a<t_i<t_b|\\&space;/\\&space;(t_b-t_a)),
  i.e. the number of events between ![t_a](https://latex.codecogs.com/svg.latex?t_a)
  and ![t_b](https://latex.codecogs.com/svg.latex?t_b) divided by the time period.
  Ultimately there will be some physical limit on bandwidth, and because each pixel
  event has higher overhead than a normal pixel (as it carries time and location, as well
  as value), the maximum events per second will be lower than the pixel rate of a frame camera.

- quality: given the event bandwidth constraints, and the amount of noise present in any
  physical sensor, event cameras need to use thresholds and event limiters to manage
  whether and event is sent. So only if a pixel changes by more than a certain threshold
  amount is an event generated, and if there are too many events in a certain period
  some of them will be delayed or lost.

Object detection
----------------

The object detection algorithm is simply looking for shapes matching a single
pattern within a video frame, and trying to track the shapes as they move
around. The pattern is given as a mask and a template, both of which are
2D matrices; the mask is used to select shapes of interest, then
the score is given as the squared difference between the shape of interest
and the mask. The best candidate object locations are identified as the
eight positions with the highest score; the reason for just eight positions is
related to the way the anticipated hardware object detector works, and to
the anticipated use model.

This is a pretty primitive way of doing template detection, but it is something
that can be made highly parallel with enough transistors, allowing for arbitrarily
high throughput. So while you may or may not propose using this algorithm
in the simulator, the effects of your method should match the proposed hardware
results.

Application usage
-----------------

The primary use-case for the client is the following:

Inputs: a base/background video stream. The current resolution is 720p HD (1280x720),
  though the ability to evaluate at different resolutions is a requirement as
  future sensors may support more pixels.

Outputs: three object event streams for the different camera:
- reference camera (3000 FPS)
- spiking camera   (3000 FPS, with at most 10MPixel/sec bandwidth)
- traditional camera (30 FPS)
The frames/per sec may vary in future cameras, and exploring the bandwidth/quality tradeoff
is a key goal of this whole application.


Process:
1. Import the base/background video stream
2. Generate a ground truth set of object positions over time
3. Combine the base video stream and ground truth objects positions into a new
    combined video stream, which reflects the "real" view to the camera.
4. Run the classifier on the full version, without modelling the spiking or
    traditional camera.
5. Model the spiking camera detecting objects \
    A - Convert the combined video stream to a spiking equivalent with a chosen camera model \
    B - Reconstruct the video stream implied by the spikes \
    C - Run the object detection filter
6. Model the traditional frame based approach \
    A - Downsample the high-speed video to simulate the slower frame rate (compared to the
        event stream) \
    B - Run the object detection filter on the slower video \
    C - Resample the event stream to map it back to the original high frame rate.


This use-case is captured in the script `scripts/standard_flow.sh`, which
takes as input a video file `XXXX.mp4` or `XXXX.mjpeg`, and produces three
files `XXXX.*.event`. It will also create a set of temporary files in
a directory called `XXXX/w`, which can be deleted after it finishes (or
you may wish to look at them).

The script supports two optional parameters which you may find useful:
```
scripts/standard_flow.sh input-video [object-stream|"random"] [frames|"all"] [oversample] [visualise]
```
- `event-stream` : Allows you to specify the input object stream, otherwise a new random stream is generated.
- `frames` : Species that only the first "frames" frames in the video should be converted (to speed things up)
- `oversample` : Used to modify the spiking to traditional framerate. By default this is 100.
- `visualise` : If this is "1" then additional videos will be generated which show the object rectangles
  events overlaid on top of the video. Was useful during debugging.

A pre-requisite is that you must have `ffmpeg` (for converting
videos to mjpeg) and `libjpeg` installed (for reading and writing
mjpeg), which are available via:
```
sudo apt install ffmpeg libjpeg-dev
```

You can run this flow using:
```
make all
scripts/standard_flow.sh media/big_buck_bunny_scene_small.mp4  random  all  10  1
```




Deliverables
============

The client has asked you to deliver a 6 page report summarising your analysis
of the application, and your proposal for a fully optimised system. They have
asked for a structured document, in order to allow them to understand each
bidders strengths and weaknesses.

The document will be read by _engineers_, which means that they will understand
technical prose, and will appreciate the use of abstraction and modelling. They
also appreciate conciseness, and the structuring of information.

This document should be committed as a single A4 or US letter-sized pdf with the path `report/report.pdf`.

While leniency will be exercised with respect to judgement of presentation, groups who produce unreasonably formatted reports may be asked, within a very short timescale, to resumbit.
Examples of unreasonable formatting include tiny font sizes, tight line spacing and very narrow margins.
A good example of a perfectly acceptable template is [the IEEETran conference format](https://www.overleaf.com/latex/templates/ieee-bare-demo-template-for-conferences/ypypvwjmvtdf).

Empirical Analysis (30%)
------------------------

A 2 page analysis which delves into the application as it currently
exists on a `g3.4xlarge` instance. This analysis should explore the performance
and bottlenecks of the application as given, and examine simple ways to
improve performance. Any modifications and improvements performed at
this stage should be included in a script called `scripts/optimised_flow.sh`.

Your analysis should:
- include at least two graphs, tables, or figures which provide data or
  evidence to support the analysis;
- explicitly refer to scripts or programs in your codebase which allowed
  you to gather results;
- reference the script `scripts/optimised_flow.sh`, and explain what modifications
  were made to support the empirical analysis.

Things that you may wish to consider include:

- Benchmarking and performance analysis.
- Profiling break-downs.
- Adding TBB, streaming parallelism, or OpenCL in low-risk / low-effort / high-impact places
  (though is unlikely one would consider OpenCL low-effort or low-risk).
- Local optimisations to improve performance and uncover real bottlenecks.

You are limited in space, so you can't describe lots of stuff. It's entirely
possible you did a number of different things, so you could just bullet-point
them with links to where they can be found in the code, then move onto
more interesting stuff.

Theoretical Analysis (30%)
--------------------------

A 2 page analysis of the theoretical/asymptotic properties of the application
from the point of view of potential parallelism and optimisation.

Your analysis should:
- include at least 1 diagram or figure to explain the application;
- make explicit reference to classes and/or functions in the original code;
- define a key performance metric that you think is important and informative.

Aspects you may wish to cover include:

- Main processes and data flows
- Data and control dependencies
- Asymptotic complexity
- Critical path and average parallelism
- Asymptotic scaling with the most important parameters
- Key bottlenecks which might limit scaling to many parallel tasks



Proposed solution (40%)
-----------------------

A 2 page proposal presenting a strategy for optimising the
application given 1 person-month worth of full-time effort. The deployment
target is the EC2 ecosystem (which instance type is up to you). You are
pitching for the resources to do the work, so your proposal should
include some sense of the risk-reward involved.

Your proposal should:
- include at least one figure, graph, table, or algorithm which supports the proposed solution;
- rely on at least one computational experiment which provides evidence for your new strategy.
- reference at least one piece of enhanced or optimised code in your codebase to provide
  credibility.
  

Things that you may wish to describe include:
- How would you solve or manage any sequential bottlenecks identified in your
  analysis?
- How does this proposal address any bottlenecks found in the empirical
  analysis?
- Are there are any micro-benchmarks or experiments that can be used to
  support the claimed approach?

The goal is not to actually implement the solution, instead you are
trying to convince the reader that:
- The approach proposed is solid;
- The performance gain is appropriate to the effort expended;
- You have good reasons for thinking the approach would solve the main problems;
- There is good technical evidence that the key techniques would work.

You may wish to consider the experience of CW5, where each kernel was not
a full application, but it did represent the core bottlneck of an
application; so providing a large speed-up for the Ising puzzle using
a GPU provides a lot of credibility to claims that you could speed up
the entire application using a GPU, even though the Ising kernel we 
looked at will be slightly different to most existing Ising simulators.

Code and data
-------------

The code you write and data you produce is part of your deliverable, but it
exists in a supporting role - it is mainly the evidence for the claims that
you make in the main document. You are not judged on the organisation or
structure of your code or repository, but it should be easy to find the
things that you used to inform your report, and you should refer to key
pieces of code or data from the report.


Possible questions
==================

So how much do we need to implement?
------------------------------------

Ideally, as little as possible.

The most value in engineering is usually contributed in analysis, design, and communication,
with implementation usually the more straightforward part. The activities in CW5 are
closer to bachelors level, as someone has already done the work of analysing the problem,
extracting the bottleneck, and saying ``make this faster''. This is a masters level course,
so we want to see the ability to analyse a somewhat complex problem, break it down into
sub-problems, and distuinguish between the important and the routine. The routine stuff
can be handled by someone else.


What speed-up is needed for an A ( or a B, ...)?
------------------------------------------------

There is no defined target for this coursework, because the aim is to provide
a proposal for a follow-on team to make it faster. Consider some different
proposals:

- We have made it 16x faster with 10 hours work. Probably it could be faster
  if some bits were moved to a GPU.

- We have made it 8x faster on 16 cores with 5 hours work, but our 5 hours of
  analysis and experimentation suggest it could be 16x faster on 32 cores
  if part X were re-written from scratch.

- We have made it 4x faster with 2 hours work, after which we identified
  a key bottleneck. Prototyping just this part for 6 hours on a GPU suggests
  that one GPU can eliminate the bottleneck for 64 CPU workers, possibly
  providing a 128x speedup with some simple but laborious refactoring.

There is an interaction between the coding and experimental work that you
do and the credibility and quality of your proposal.


So... we don't need to write any code?
--------------------------------------

Well, maybe not, but if you claim that using a GPU would make some aspect
of the application 100x faster, but have no evidence of actually trying it,
your claim looks rather weak.


So... our code doesn't need to run?
-----------------------------------

We may ask pairs to demonstrate or explain how they got results, particularly
in cases where:
- It is unclear where particular performance figures came from; or
- code that is used to provide figures doesn't appear correct; or
- the results of experiments don't make sense.
Trying to link clearly from the report back to the code and/or
original data can help here, particularly if you have organised your
main experiments.

The default positition is that anything you say in the report was done
and works, but one part of assessment is checking whether assertions made
are correct. We don't expect to see anyone making things up, but it's something
we need to defend against to ensure robustness, just like checking for
plagiarism in the code or report.

So we don't expect to use this option, but just because the assessment doesn't
say exactly what code will be run, don't assume the code never needed to run.


How do we start?
----------------

Consider the content of the course: e.g. metrics, complexity, profiling, performance
analysis, critical paths, adding parallelism to existing code, transforming for parallelism,
and so on. You have a set of implementation techniques that includes TBB and OpenCL, but if
you've been paying attention it's not the only part of your toolbox.

The goal of the course (and your degree) is not to learn any particular framework or technology,
the idea is that you change the way that you think and approach problems. 


How much time is needed?
------------------------

You could estimate this according to ECTS, or some other metric, but ultimately
it's up to you and we can't control it. Practically speaking it has to be less
than 1000 hours for a pair :)

You should _decide_ how much time to spend, and then spend that time budget as makes
sense. You've got three deliverable sections, each of which needs to be written
up with pictures/graphs. Reading this document properly has probably already taken
close to an hour, so that time is gone. Hopefully you know your speed of writing and
documentation, and can use that to allocate a time budget for producing 3 pages of
text and a number of figures. Whatever is left over you should allocate to the
different activities you need to do. Note that the deliverable split is intended
to help you stay on track and avoid over-allocating on just one activity, so
ideally you would set a rough time budget for yourself and stick to it.


Can we use external library X?
------------------------------

The client is really only interested in standalone C++ applications, with the addition
of TBB and OpenCL, as that is what they are comfortable with maintaining. You
might propose using methods from an external library, but you'd need to explain
how they would be implemented in this specific application. So If you suggest
"use function X from OpenCV" then you haven't really solved the clients problem. Similarly,
if you say "use algorithm Y from paper ABC", you aren't really convincing that _you_ (and your
team) could actually do it. However, you might use external libraries plus some analysis
and descriptions of their internals to support the claim that you 


Can we use existing code?
-------------------------

If you want to use code from any external source then you need to clearly indicate
and attribute it in the source code, and also highlight it in the report. The client
will only accept code with OSI licenses, and the appropriate license should be
clearly attached within the code-base.


If we run big files we run out of disk space in EC2
---------------------------------------------------

Indeed. But do you need to?


The signal/image processing is really dumb, why not use algorithm X?
--------------------------------------------------------------------

Yes, it is very naive. But remember that it is supposed to be modelling
custom parallel hardware able to process at ~3000 FPS with sub ms
latency. The sophisticated methods can achieve high throughputs, but
usually not at the same time as low latency. Also, this isn't a signals
course, so it has to be somewhat simplified to be manageable in this
time-frame - do you really suggest making the coursework more complex :)


The spiking camera doesn't seem to model the real cameras
---------------------------------------------------------

Yes, the processing model is a bit different, and we don't model things
like the dynamic range correctly (which is a key feature of event-driven
cameras), nor the way that they manager bandwidth. However, it is close
enough to provide a useful quality-latency tradeoff curve.


We're still not sure how to start...
------------------------------------

A good place to start is by asking yourself "what is slow?" This requires you
to come up with some initial definition of "slowness", and then to try to 
measure how slow the current solution is. You can then start to probe which
parts are making it slow. At that point you can start to work somewhat in
parallel: empirically probing into the data-flow and computational-load
in the application, while analytically trying to capture the structure
and behaviour of the application based on what you see.


Are these spiking cameras real?
-------------------------------

Indeed they are, though they are still mostly academic. For example, there has been some
work at Imperial in both EEE and DoC which investigates them, particularly
[in the context of SLAM](http://www.doc.ic.ac.uk/~hkim1/Publications/kim_etal_bmvc2014.pdf).
In addition to the low-latency feature explored here, they have some really nice features
related to dynamic range and power consumption which makes them attractive for robotics. There has also been some
more recent work which tries to combine traditional sensors and event sensors,
using [event cameras to fill in the gaps between standard frames](https://www.youtube.com/watch?v=A7UfeUnG6c4).

As it happens, the use-case presented here is actually based on a real research project,
except the back-end is a full ML algorithm loosely based on AlexNet. Rather than doing
the detection in parallel, it would incrementally update those parts of the neural network
that change in response to each spike Before investing any time in actually doing it (which
would involve both buying and event camera and building some custom hardware), we needed to
work out how much of the network changes on average in real scenarios, so we had to try to
model typical spiking behaviour. So this application is not that code, but is an abstract
version of a computational problem that occurred in practise.


Why not use a higher frame rate standard camera?
------------------------------------------------

True, in reality you can get COTS cameras which support 120 FPS quite easily, and we could
sustain 720p quite easily at that rate. Some DSLR-style sensors go up to ~500FPS in HD,
and are still 720p-ish up to 1000FPS, but they start to be burst limited, and just
dump frames into RAM until RAM fills up. Doing any kind of sustained workload at
1000FPS is still pretty difficult, and going much above 1000 FPS starts to get expensive
and quite specialised. In comparison, the real event cameras have resolutions approaching
10 us, which is two orders of magnitude faster again. So while there are methods for going
very fast with frame based cameras, achieving sustained throughput and low-latency at
the same time are still very difficult, so event cameras remain intruiging.

