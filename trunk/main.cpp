#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <assert.h>

//********************************************************************************
// Demonstrates how to use the JobSwarm system.
//********************************************************************************

//#define STRESS_TEST
#include "UserMemAlloc.h"
#include "ThreadConfig.h"
#include "sgif.h"
#include "JobSwarm.h"

#define FRACTAL_SIZE 2048
#define NUM_THREADS 8

#ifndef STRESS_TEST

   #define SWARM_SIZE 8
   #define MAX_ITERATIONS 65536

   #define SPOOL_JOBS 0    // if true, causes the jobs to be spooled over time, instead of created all at once, this causes race conditions
   #define SPOOL_JOB_SIZE 256 // how many jobs allowed to be active/spooled at once.

#else

   // stress test conditions

   #define SWARM_SIZE 2
   #define MAX_ITERATIONS 16

   #define SPOOL_JOBS 1    // if true, causes the jobs to be spooled over time, instead of created all at once, this causes race conditions
   #define SPOOL_JOB_SIZE 32 // how many jobs allowed to be active/spooled at once.

#endif

//********************************************************************************
// solves a single point in the mandelbrot set.
//********************************************************************************
static inline unsigned int mandelbrotPoint(unsigned int iterations,double real,double imaginary)
{
	double fx,fy,xs,ys;
	unsigned int count;

  double two(2.0);

	fx = real;
	fy = imaginary;
  count = 0;

  do
  {
    xs = fx*fx;
    ys = fy*fy;
		fy = (two*fx*fy)+imaginary;
		fx = xs-ys+real;
    count++;
	} while ( xs+ys < 4.0 && count < iterations);

	return count;
}

static inline unsigned int solvePoint(unsigned int x,unsigned int y,double x1,double y1,double xscale,double yscale)
{
  return mandelbrotPoint(MAX_ITERATIONS,(double)x*xscale+x1,(double)y*yscale+y1);
}

//********************************************************************************
// solves the fractal using a single core/single thread in a single set for/next loops
//********************************************************************************
unsigned int fractalLinear(void)
{

	unsigned int stime = THREAD_CONFIG::tc_timeGetTime();

  double  x1 = -0.56017680903960034334758968;
  double  x2 = -0.5540396934395273995800156;
  double  y1 = -0.63815211573948702427222672;
  double  y2 = y1+(x2-x1);

  double   xscale = (x2-x1)/(double)FRACTAL_SIZE;
  double   yscale = (y2-y1)/(double)FRACTAL_SIZE;

  unsigned char *fractal = MEMALLOC_NEW_ARRAY(unsigned char,FRACTAL_SIZE*FRACTAL_SIZE)[FRACTAL_SIZE*FRACTAL_SIZE];
  unsigned char *dest = fractal;
  for (unsigned int y=0; y<FRACTAL_SIZE; y++)
  {
    for (unsigned int x=0; x<FRACTAL_SIZE; x++)
    {
      unsigned int v = solvePoint(x,y,x1,y1,xscale,yscale);
      if ( v == MAX_ITERATIONS )
        v = 0;
      else
        v = v&0xFF;
      *dest++ = (char)v;
    }
  }
  unsigned int etime = THREAD_CONFIG::tc_timeGetTime();

  printf("Saving fractal image as 'fractal_linear.gif'\r\n");
  saveGIF("fractal_linear.gif",FRACTAL_SIZE,FRACTAL_SIZE,0,fractal);
  MEMALLOC_DELETE_ARRAY(unsigned char,fractal);
  return etime-stime;
}


//********************************************************************************
// A small class to handle each individual 'job' to solve the fractal.
//********************************************************************************
class FractalJob : public JOB_SWARM::JobSwarmInterface
{
public:
  FractalJob(void)
  {
  }

  void init(JOB_SWARM::JobSwarmContext *c,unsigned int x1,unsigned int y1,double fx,double fy,double xscale,double yscale,unsigned char *dest,unsigned int *counter)
  {
    mX1 = x1;
    mY1 = y1;
    mFX = fx;
    mFY = fy;
    mXscale = xscale;
    mYscale = yscale;
    mCounter = counter;
    c->createSwarmJob(this,dest,0);
  }

  virtual void job_process(void * userData,int /* userId */)    // RUNS IN ANOTHER THREAD!! MUST BE THREAD SAFE!
  {
    unsigned char *fractal_image = (unsigned char *)userData;
    for (unsigned int y=0; y<SWARM_SIZE; y++)
    {
      unsigned int index = (y+mY1)*FRACTAL_SIZE+mX1;
      unsigned char *dest = &fractal_image[index];
      for (unsigned int x=0; x<SWARM_SIZE; x++)
      {
        unsigned int v = solvePoint(x+mX1,y+mY1,mFX,mFY,mXscale,mYscale);
        if ( v == MAX_ITERATIONS )
          v = 0;
        else
          v = v&0xFF;
        *dest++ = (char)v;
      }
    }
  }

  virtual void job_onFinish(void * /* userData */,int /* userId */)  // runs in primary thread of the context
  {
    (*mCounter)--;
  }

  virtual void job_onCancel(void * /* userData */,int /* userId */)   // runs in primary thread of the context
  {
    (*mCounter)--;
  }

private:
  unsigned int mX1;
  unsigned int mY1;
  double mFX;
  double mFY;
  double mXscale;
  double mYscale;
  unsigned int *mCounter;
};

unsigned int fractalJobSwarm(void)
{

  JOB_SWARM::JobSwarmContext *context = JOB_SWARM::createJobSwarmContext( NUM_THREADS );

  unsigned int taskRow = FRACTAL_SIZE/SWARM_SIZE;
  unsigned int taskCount = taskRow*taskRow;
  printf("Solving fractal as %d seperate jobs.\r\n", taskCount );

	unsigned int stime = THREAD_CONFIG::tc_timeGetTime();

  FractalJob *jobs = MEMALLOC_NEW_ARRAY(FractalJob,taskCount)[taskCount]; // allocate memory for all of the sub-tasks

  double  x1 = -0.56017680903960034334758968;
  double  x2 = -0.5540396934395273995800156;
  double  y1 = -0.63815211573948702427222672;
  double  y2 = y1+(x2-x1);

  double   xscale = (x2-x1)/(double)FRACTAL_SIZE;
  double   yscale = (y2-y1)/(double)FRACTAL_SIZE;

  unsigned char *fractal = MEMALLOC_NEW_ARRAY(unsigned char,FRACTAL_SIZE*FRACTAL_SIZE)[FRACTAL_SIZE*FRACTAL_SIZE];

  FractalJob *next_job = jobs;

  unsigned int taskRemaining = 0;

  for (unsigned int y=0; y<FRACTAL_SIZE; y+=SWARM_SIZE)
  {
    for (unsigned int x=0; x<FRACTAL_SIZE; x+=SWARM_SIZE)
    {
      taskRemaining++;
      next_job->init(context,x,y,x1,y1,xscale,yscale,fractal,&taskRemaining);
			next_job++;
#if SPOOL_JOBS
      while ( taskRemaining > SPOOL_JOB_SIZE ) // halt posting new jobs while there are more than 2k jobs outstanding
      {
        context->processSwarmJobs();
      }
#endif
    }
  }

  // ok..now we wait until the task remaining counter is zero.
  while ( taskRemaining != 0 )
  {
    context->processSwarmJobs();
  }


  unsigned int etime = THREAD_CONFIG::tc_timeGetTime();
  printf("Saving fractal image as 'fractal_swarm1.gif'\r\n");
  saveGIF("fractal_swarm1.gif",FRACTAL_SIZE,FRACTAL_SIZE,0,fractal);

  MEMALLOC_DELETE_ARRAY(FractalJob,jobs);
  MEMALLOC_DELETE_ARRAY(unsigned char,fractal);

  JOB_SWARM::releaseJobSwarmContext(context);

  return etime-stime;
}

//***************************************************************************************
//*** Solves the fractal using the jobswarm, but with a single class for dispatching work.
//***************************************************************************************
class FractalSolver : public JOB_SWARM::JobSwarmInterface
{
public:
  unsigned int solveFractal(void)
  {
    JOB_SWARM::JobSwarmContext *context = JOB_SWARM::createJobSwarmContext( NUM_THREADS );

    unsigned int taskRow = FRACTAL_SIZE/SWARM_SIZE;
    unsigned int taskCount = taskRow*taskRow;
    printf("Solving fractal as %d seperate jobs with a single callback location.\r\n", taskCount );

  	unsigned int stime = THREAD_CONFIG::tc_timeGetTime();

    mX1 = -0.56017680903960034334758968;
    mX2 = -0.5540396934395273995800156;
    mY1 = -0.63815211573948702427222672;
    mY2 = mY1+(mX2-mX1);

    mXscale = (mX2-mX1)/(double)FRACTAL_SIZE;
    mYscale = (mY2-mY1)/(double)FRACTAL_SIZE;

    mFractal = MEMALLOC_NEW_ARRAY(unsigned char,FRACTAL_SIZE*FRACTAL_SIZE)[FRACTAL_SIZE*FRACTAL_SIZE];

    mTasksRemaining = 0;

    for (unsigned int y=0; y<FRACTAL_SIZE; y+=SWARM_SIZE)
    {
      for (unsigned int x=0; x<FRACTAL_SIZE; x+=SWARM_SIZE)
      {
        unsigned int index = y*FRACTAL_SIZE+x;
        mTasksRemaining++;
        context->createSwarmJob(this,0,index);
#if SPOOL_JOBS
        while ( mTasksRemaining > SPOOL_JOB_SIZE ) // don't post any new jobs while there are more than 2k jobs outstanding...
        {
          context->processSwarmJobs();
        }
#endif
      }
    }

    // ok..now we wait until the task remaining counter is zero.
    while ( mTasksRemaining != 0 )
    {
      context->processSwarmJobs();
    }


    unsigned int etime = THREAD_CONFIG::tc_timeGetTime();

    printf("Saving fractal image as 'fractal_swarm2.gif'\r\n");
    saveGIF("fractal_swarm2.gif",FRACTAL_SIZE,FRACTAL_SIZE,0,mFractal);

    MEMALLOC_DELETE_ARRAY(unsigned char,mFractal);

    JOB_SWARM::releaseJobSwarmContext(context);

    return etime-stime;

  }

  // from the UserId field we get the address in the fractal to solve
  virtual void job_process(void * /*userData */,int userId)    // RUNS IN ANOTHER THREAD!! MUST BE THREAD SAFE!
  {
    unsigned int x1 = userId%FRACTAL_SIZE;
    unsigned int y1 = userId/FRACTAL_SIZE;

    for (unsigned int y=0; y<SWARM_SIZE; y++)
    {
      unsigned int index  = (y+y1)*FRACTAL_SIZE+x1;
      unsigned char *dest = &mFractal[index];
      for (unsigned int x=0; x<SWARM_SIZE; x++)
      {
        unsigned int v = solvePoint(x+x1,y+y1,mX1,mY1,mXscale,mYscale);
        if ( v == MAX_ITERATIONS )
          v = 0;
        else
          v = v&0xFF;
        *dest++ = (char)v;
      }
    }
  }

  virtual void job_onFinish(void * /* userData */,int /* userId */)  // runs in primary thread of the context
  {
    mTasksRemaining--;
  }

  virtual void job_onCancel(void * /* userData */,int /* userId */)   // runs in primary thread of the context
  {
    mTasksRemaining--;
  }

private:
  unsigned int mTasksRemaining;
  double       mX1;
  double       mX2;
  double       mY1;
  double       mY2;
  double       mXscale;
  double       mYscale;
  unsigned char *mFractal;
};

void main(int /* argc */,const char ** /* argv */)
{

   _controlfp(_PC_64,_MCW_PC); // set the floating point control word for full 64 bit precision.

   #ifndef STRESS_TEST

      printf("Solving a fractal using one core and one thread in a straight line computation.\r\n");
      unsigned int t = fractalLinear();
      printf("Took %d milliseconds to compute the fractal using one core without threading.\r\n", t );
      printf("\r\n");

      t = fractalJobSwarm();
      printf("Took %d milliseconds to compute the fractal using a job swarm.\r\n", t );
      printf("\r\n");

      FractalSolver f;
      t = f.solveFractal();
      printf("Took %d milliseconds to compute the fractal using a job swarm with a single callback class.\r\n",t);
      printf("\r\n");

   #else

      const int    iterations=10;
      //
      printf("settings:\n  NUM_THREADS %d\n  SWARM_SIZE %d\n  MAX_ITERATIONS %d\n  SPOOL_JOBS %d\n  SPOOL_JOB_SIZE %d\n" , 
               NUM_THREADS, SWARM_SIZE, MAX_ITERATIONS, SPOOL_JOBS, SPOOL_JOB_SIZE );
      //
      unsigned int sum=0;
      //
      for(int a=0;a<iterations;a++)
      {
         printf( "%d/%d\n" , a , iterations );
         unsigned int t = fractalJobSwarm();
         //
         printf("  Took %d milliseconds to compute the fractal using a job swarm.\r\n", t );
         //
         sum+=t;
      }
      //
      printf( "Average time of %d tests %d\n" , iterations , sum / iterations );

   #endif

}

