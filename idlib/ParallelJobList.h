#ifndef __PARALLELJOBLIST_H__
#define __PARALLELJOBLIST_H__

struct CellSpursJob128;
class idColor;

typedef void ( * jobRun_t )( void * );

enum jobSyncType_t {
	SYNC_NONE,
	SYNC_SIGNAL,
	SYNC_SYNCHRONIZE
};

// NOTE: keep in sync with jobNames[]
enum jobListId_t {
	JOBLIST_RENDERER_FRONTEND	= 0,
	JOBLIST_RENDERER_BACKEND	= 1,
	JOBLIST_UTILITY				= 9,			// won't print over-time warnings

	MAX_JOBLISTS				= 32			// the editor may cause quite a few to be allocated
};

compile_time_assert( CONST_ISPOWEROFTWO( MAX_JOBLISTS ) );

enum jobListPriority_t {
	JOBLIST_PRIORITY_NONE,
	JOBLIST_PRIORITY_LOW,
	JOBLIST_PRIORITY_MEDIUM,
	JOBLIST_PRIORITY_HIGH
};

enum jobListParallelism_t {
	JOBLIST_PARALLELISM_DEFAULT			= -1,	// use "jobs_numThreads" number of threads
	JOBLIST_PARALLELISM_MAX_CORES		= -2,	// use a thread for each logical core (includes hyperthreads)
	JOBLIST_PARALLELISM_MAX_THREADS		= -3	// use the maximum number of job threads, which can help if there is IO to overlap
};

#define assert_spu_local_store( ptr )
#define assert_not_spu_local_store( ptr )

/*
================================================
idParallelJobList

A job should be at least a couple of 1000 clock cycles in
order to outweigh any job switching overhead. On the other
hand a job should consume no more than a couple of
100,000 clock cycles to maintain a good load balance over
multiple processing units.
================================================
*/
class idParallelJobList {
	friend class idParallelJobManagerLocal;
public:

	void					AddJob( jobRun_t function, void *data );
	CellSpursJob128 *		AddJobSPURS();
	void					InsertSyncPoint( jobSyncType_t syncType );

	// Submit the jobs in this list.
	void					Submit( idParallelJobList *waitForJobList = nullptr, int parallelism = JOBLIST_PARALLELISM_DEFAULT );
	// Wait for the jobs in this list to finish. Will spin in place if any jobs are not done.
	void					Wait();
	// Try to wait for the jobs in this list to finish but either way return immediately. Returns true if all jobs are done.
	bool					TryWait();
	// returns true if the job list has been submitted.
	bool					IsSubmitted() const;

	// Get the number of jobs executed in this job list.
	unsigned int			GetNumExecutedJobs() const;
	// Get the number of sync points.
	unsigned int			GetNumSyncs() const;
	// Time at which the job list was submitted.
	unsigned int			GetSubmitTimeMicroSec() const;
	// Time at which execution of this job list started.
	unsigned int			GetStartTimeMicroSec() const;
	// Time at which all jobs in the list were executed.
	unsigned int			GetFinishTimeMicroSec() const;
	// Time the host thread waited for this job list to finish.
	unsigned int			GetWaitTimeMicroSec() const;
	// Get the total time all units spent processing this job list.
	unsigned int			GetTotalProcessingTimeMicroSec() const;
	// Get the total time all units wasted while processing this job list.
	unsigned int			GetTotalWastedTimeMicroSec() const;
	// Time the given unit spent processing this job list.
	unsigned int			GetUnitProcessingTimeMicroSec( int unit ) const;
	// Time the given unit wasted while processing this job list.
	unsigned int			GetUnitWastedTimeMicroSec( int unit ) const;

	// Get the job list ID
	jobListId_t				GetId() const;
	// Get the color for profiling.
	const anVec4 *			GetColor() const { return this->color; }

private:
	class idParallelJobList_Threads *	jobListThreads;
	const anVec4 *						color;

	idParallelJobList( jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs, const anVec4 * color );
	~idParallelJobList();
};

/*
================================================
idParallelJobManager

This is the only interface through which job lists
should be allocated or freed.
================================================
*/
class idParallelJobManager {
public:
	virtual						~idParallelJobManager() {}

	virtual void				Init() = 0;
	virtual void				Shutdown() = 0;

	virtual idParallelJobList *	AllocJobList( jobListId_t id, jobListPriority_t priority, unsigned int maxJobs, unsigned int maxSyncs, const idColor * color ) = 0;
	virtual void				FreeJobList( idParallelJobList * jobList ) = 0;

	virtual int					GetNumJobLists() const = 0;
	virtual int					GetNumFreeJobLists() const = 0;
	virtual idParallelJobList *	GetJobList( int index ) = 0;

	virtual int					GetNumProcessingUnits() = 0;

	virtual void				WaitForAllJobLists() = 0;
};

extern idParallelJobManager *	parallelJobManager;

// jobRun_t functions can have the debug name associated with them
// by explicitly calling this, or using the REGISTER_PARALLEL_JOB()
// static variable macro.
void RegisterJob( jobRun_t function, const char * name );

/*
================================================
idParallelJobRegistration
================================================
*/
class idParallelJobRegistration {
public:
	idParallelJobRegistration( jobRun_t function, const char * name );
};

#define REGISTER_PARALLEL_JOB( function, name ) static idParallelJobRegistration register_##function( (jobRun_t) function, name )

#endif // !__PARALLELJOBLIST_H__
