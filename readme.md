Overview
========

This code-base describes a basic test-bench which is intended to
evaluate the performance of a traditional frame-based classifier
against an event-driven classifier. The overall context is that
someone is interested in identifying known patterns in video
streams, but wants to do so with very low latency (sub-ms)
with large frame sizes. Their choice of solution then looks like:

- Traditional image capture using a 30 FPS high-speed camera. 
  Each frame is streamed off for processing, with a minimum
  detection time of at least 70ms: 33ms to capture the frame,
  20 ms to get it off the camera, and 20 ms to perform full-frame
  object detection.

- Even-driven capture using a spiking camera, which produces a stream
  of events describing when individual pixels change. The event stream
  is processed progressively as the spikes are received, reducing
  the latency to less than 1ms. They know how to build custom hardware
  object detection which is able to perform object detection in a
  massively parallel well, but are limited in how many spikes per
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
with the investment of a lot more person-hours. A by-product of this
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
fixed time period $delta_f$, so it produces a sequence of frames:
```
f_0, f_1, f_2, ...
```
where frame $f_i$ is associated with the relative time point $i*delta_f$,
and each frame is a $w \times h \times c$ tensor (well, technically a holor).
Typically $c=1$ for gray-scale or $c=3$ for RGB or YUV.

The event based cameras being considered produce a stream of pixel events,
which describe changes to pixel intensity at specific points in time. These
events occur at a sequence of (non-strict) monotonically increasing
time points, and each event carries an update to exactly one pixel. So we
have a sequence of events:
```
e_0, e_1, e_2, ...
```
with each event a tuple $e_i=(t,x,y,v)$ where:
- $t$ is when the event occurred
- $(x,y)$ is the pixel co-ordinate being updated, with $0 \leq x < w$ and $0 \leq y < h$.
- v is a $c$ component vector, e.g. for RGB is a 3-dimensional vector.
Time monotonicity means that we should always have $i < j \implies t_i \leq t_j$.

From the spike sequence we can convert back to complete frames at
each discrete event time, though it would produce an extremely
high frame-rate stream with relatively low information content
(as only one pixel changes per frame).

The relative advantages of frame-based and event-based camers comes down
to a tradeoff between:

- latency: the frame-based camera has a fixed temporal resolution of $delta_f$,
  while the temporal resolution of the event-based camera depends on how quickly
  pixels are changing, and how many pixels are changing.

- bandwidth: the frame-based camera requires fixed band-width of $O( w \times h \times c / delta_f )$
  while the event-based camera bandwidth varies with over time; between time points
  $t_a$ and $t_b$ the bandwidth will be proportional to $| { e_i : t_a < t_i < t_b } | / (t_b-t_a)$,
  i.e. the number of events between $t_a$ and $t_b$ divided by the time period.
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
related to the way the anticipated hardware object detector works.

To cater for the fact that objects might appear with different sizes (or at
different distances from the camera) the process is repeated at a number of
different resolutions. Each coarser resolution is created by halving the
image dimensions using averaging, and then performing the search again. Matches
can then occur both at different positions, and at different scales.

This is a pretty primitive way of doing template detection, but it is something
that can be made highly parallel with enough transistors, allowing for arbitrarily
high throughput. So while you may or may not propose using this algorithm
in the simulator, the effects of your method should match the proposed hardware.

Application usage
-----------------

The primary use-case for the client is the following:
1 - Take a base/background video stream
2 - Generate a ground truth set of object positions over time
3 - Combine the base video stream and ground truth objects positions into a new
    combined video stream.
4 - Model the spiking camera detecting objects
    A - Convert the combined video stream to a spiking equivalent with a chosen camera model
    B - Reconstruct the video stream implied by the spikes
    C - Run the object detection filter
5 - Model the traditional frame based approach
    A - Downsample the high-speed video to simulate a slower frame rate (compared to the
        event stream).
    B - Run the object detection filter
6 - Combine the ground truth positions with the results from the spiking and
    traditional results, producing a single file describing when and where
    objects were detected.

This specific use-case is captured in the script `scripts/standard_analysis.sh`, which
takes as input a video, and produces a combined file of positions.

Deliverables
============

The client has asked you to deliver a 6 page report summarising your analysis
of the application, and your proposal for a fully optimised system. They have
asked for a structured document, in order to allow them to understand each
bidders strengths and weaknesses.

Empirical Analysis (30%)
------------------------

A 2 page analysis which delves into the application as it currently
exists on a g3.4xlarge instance. This analysis should explore the performance
and bottlenecks of the application as given, and examine simple ways to
improve performance.

Your analysis should:
- contain at most 1 page worth of text;
- include at least two graphs, tables, or figures which provide data or
  evidence to support the analysis;
- explicitly refer to scripts or programs in your codebase which allowed
  you to gather results. 

Things that you may wish to consider include:

- Benchmarking and performance analysis
- Profiling break-downs
- Adding TBB, streaming parallelism, or OpenCL in low-risk high-impact places
- Local optimisations to improve performance and uncover real bottlenecks.


Theoretical Analysis (30%)
--------------------------

A 2 page analysis of the theoretical/asymptotic properties of the application
from the point of view of potential parallelism and optimisation.

Your analysis should:
- contain at most 1 page worth of text;
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
- contain at most 1 page worth of text;
- include at least one figure, graph, table, or algorithm which supports the proposed solution;
- rely on at least one experiment which provides evidence for your new strategy.
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
the entire application using a GPU, even though the Ising kernel will
be slightly different to most existing Ising simulators.

Code and data
-------------

The code you write and data you produce is part of your deliverable, but it
exists in a supporting role - it is mainly the evidence for the claims that
you make in the main document. You are not judged on the organisation or
structure of your code or repository, but it should be easy to find the
things that you used to inform your report.


Meta-comments
=============

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
