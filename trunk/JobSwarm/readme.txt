JobSwarm Released January 2, 2009 by John W. Ratcliff

This is a test framework for micro-threading applications.

This is a project I intend to maintain over time.  I am looking for contributions from
other developers along the way.

This project demonstrates the power of 'micro-threading'.  What I mean by this is
breaking a problem up into many tiny 'jobs' each one of which can be executed in a thread.

A system called the 'JobSwarm' manages these jobs executing on multiple physical threds.

The overhead for processing a job is so low that you can execute tens upon tens of thousands
of tiny jobs while still getting full processor utilization on multiple cores and even
hyper-threaded machines.

This demo program computes a fractal in two ways.  First, in a straight linear fashion
using one thread and one core.

Next it will tear the fractal up into 65,536 seperate 'jobs' each representing an 8x8 grid
or 64 pixels of the fractal.

On a four core machine, it runs almost 3.6 times faster and makes 100% processor utilization.
The main reason it doesn't run exactly 4 times faster is that the linear version has better
cache coherency.


The output is a 2048x2048 GIF image called 'fractal_linear.gif', 'fractal_swarm1.gif' and 'fractal_swarm2.gif'

------------------------------

Important notes for latest release January 3, 2009
-----------------------------------------------
* Removed fractal.cpp and fractal.h
* Revised the 'main.cpp' to be a much easier to follow demo program.
* Made two examples of using the JobSwarm system
    - First where every job has its own class for callback notification
    - Second where there is one central class for all callbacks and it does the work individually
* Added support for a user data pointer and user data id number when posting jobs.
* Fixed a crash on exit because the shutdown code for the threads was not clean previously.
* Built with VC2008.  If you have an earlier version of Visual Studio jsut create a new console project and add the source to it.


-----------------------------------------------
* Changes for January 4, 2009 release
*
* Created project and solution files for VC2003, VC2005, and VC2008
*
* Adding project to GitLib
*
* Integrated a true lock-free queue based on code provided by David Pangerl
* from the Algorithms list.  I think this code might possibly still have a
* thread locking issue.
*
* To switch back the safe, but slower, locked FIFO change the #define at the
* top of LockFreeQ.cpp
*


Release notes for January 5, 2009
-----------------------------------------------
* Integrated the latest version of the LockFreeQ by David Pangerl
* Added an additional conditional compilation to spool the jobs over time, causing race conditions
    This change exposed a problem in the lock-free queue.
    To reproduce it, load MAIN.cpp and set SPOOL_JOBS to 1


Release notes for January 6, 2009
-----------------------------------------------------------
* Disabled David Pangrl's code by default, because it has a bug in it.
* Added the source to SourceForge


Release notes for January 7, 2009
-------------------------------------------------------
* Integrated David Pangrl's latest code for the lockfree queue.  Appears to be working fine now.
* This project is now officially hosted at source forge at the location:
         http://sourceforge.net/projects/jobswarm/

* If you would like to be a contributing member to this small project, please email me at mailto:jratcliffscarab@gmail.com

Release notes for January 10, 2009
------------------------------------------------------------
* Integrated David Pengerl's latest code changes.  Thanks David!!!
* Added the UserMemAlloc.h header file which affords the opportunity for a developer to trap all memory allocations performed by the system.

