#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#define CSV

unsigned int seed = 0;
int NowYear;  // 2024- 2029
int NowMonth; // 0 - 11

float NowPrecip; // inches of rain per month
float NowTemp;   // temperature this month
float NowHeight; // grain height in inches
int NowNumDeer;  // number of deer in the current population
int NowNumHunter;
const float GRAIN_GROWS_PER_MONTH = 12.0;
const float ONE_DEER_EATS_PER_MONTH = 1.0;

const float AVG_PRECIP_PER_MONTH = 7.0; // average
const float AMP_PRECIP_PER_MONTH = 6.0; // plus or minus
const float RANDOM_PRECIP = 2.0;        // plus or minus noise

const float AVG_TEMP = 60.0;    // average
const float AMP_TEMP = 20.0;    // plus or minus
const float RANDOM_TEMP = 10.0; // plus or minus noise

const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0;

omp_lock_t Lock;
volatile int NumInThreadTeam;
volatile int NumAtBarrier;
volatile int NumGone;
// functions
void InitBarrier(int n);
void WaitBarrier();
float SQR(float x);
void Deer();
void Grain();
void Watcher();
void MyAgent();
float Ranf(float low, float high);
void findTemperature(int month);
void findPrecipitation(int month);

int main(int argc, char const *argv[])
{
#ifdef _OPENMP
    fprintf(stderr, "OpenMP is supported -- version = %d\n", _OPENMP);
#else
    fprintf(stderr, "No OpenMP support!\n");
    return 1;
#endif

    srand(seed);
    // starting date and time:
    NowMonth = 0;
    NowYear = 2024;

    // starting state (feel free to change this if you want):
    NowNumDeer = 10;
    NowHeight = 40;
    NowNumHunter = 2;

    findPrecipitation(NowMonth);
    findTemperature(NowMonth);


    omp_init_lock(&Lock);
    InitBarrier(4);
    omp_set_num_threads(4); // same as # of sections
    
#pragma omp parallel sections
    {
#pragma omp section
        {
            Deer();
        }

#pragma omp section
        {
            Grain();
        }

#pragma omp section
        {
            Watcher();
        }

#pragma omp section
        {
            MyAgent(); // your own
        }
    } // implied barrier -- all functions must return in order
    // to allow any of them to get past here

    return 0;
}
void InitBarrier(int n)
{
    NumInThreadTeam = n;
    NumAtBarrier = 0;
    omp_init_lock(&Lock);
}

void WaitBarrier()
{
    omp_set_lock(&Lock);
    {
        NumAtBarrier++;
        if (NumAtBarrier == NumInThreadTeam)
        {
            NumGone = 0;
            NumAtBarrier = 0;
            // let all other threads get back to what they were doing
            // before this one unlocks, knowing that they might immediately
            // call WaitBarrier( ) again:
            while (NumGone != NumInThreadTeam - 1)
                ;
            omp_unset_lock(&Lock);
            return;
        }
    }
    omp_unset_lock(&Lock);

    while (NumAtBarrier != 0)
        ; // this waits for the nth thread to arrive

#pragma omp atomic
    NumGone++; // this flags how many threads have returned
}

float SQR(float x)
{
    return x * x;
}

void Deer()
{
    while (NowYear < 2030)
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        int nextNumDeer = NowNumDeer;
        int carryingCapacity = (int)(NowHeight);
        if (nextNumDeer < carryingCapacity)
            nextNumDeer++;
        else if (nextNumDeer > carryingCapacity)
            nextNumDeer--;

        if (nextNumDeer < 0)
            nextNumDeer = 0;

        // DoneComputing barrier:
        WaitBarrier();
        NowNumDeer = nextNumDeer; // update NowNumDeer to nextNumDeer
        // DoneAssigning barrier:
        WaitBarrier();
        // DonePrinting barrier:
        WaitBarrier();
    }
}
void Grain()
{
    while (NowYear < 2030)
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.));
        float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.));
        float nextHeight = NowHeight;
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        if (nextHeight < 0.)
            nextHeight = 0.;
        // DoneComputing barrier:
        WaitBarrier();
        NowHeight = nextHeight;

        // DoneAssigning barrier:
        WaitBarrier();
        // DonePrinting barrier:
        WaitBarrier();
    }
}
void Watcher()
{
    while (NowYear < 2030)
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:

        // DoneComputing barrier:
        WaitBarrier();
        // DoneAssigning barrier:
        WaitBarrier();
#ifdef CSV
        fprintf(stderr, "%2d , %2d , %6.2f , %d , %d , %6.2f , %6.2f\n",
                NowYear, NowMonth, NowHeight, NowNumDeer, NowNumHunter, NowTemp, NowPrecip);
#else
        fprintf(stderr, "Year: %2d , Month: %2d , Grain Height: %7.2f , Deer: %d , Hunter: %d , Temp: %6.2f , Precip: %6.2f\n",
                NowYear, NowMonth, NowHeight, NowNumDeer, NowNumHunter, NowTemp, NowPrecip);
#endif

        if (NowMonth < 11)
        {
            NowMonth++;
        }
        else
        {
            NowYear++;

            NowMonth = 0;
        }
        findPrecipitation(NowMonth);
        findTemperature(NowMonth);
        // DonePrinting barrier:
        WaitBarrier();
    }
}
void MyAgent()
{
    while (NowYear < 2030)
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        int grainPlanted = NowNumHunter* 2;

        // DoneComputing barrier:
        WaitBarrier();
        if (NowNumDeer/2 > NowNumHunter && NowNumHunter >= 0)
        {
            NowNumHunter++;
        }else
        {
            NowNumHunter--;
        }
        NowNumDeer--;
        NowHeight += grainPlanted - NowNumHunter;//each hunter plants doupbe what they consume. 
        if(NowNumHunter < 0 ){
            NowNumHunter = 0;
        }
        // DoneAssigning barrier:
        WaitBarrier();

        // DonePrinting barrier:
        WaitBarrier();
    }
}

float Ranf(float low, float high)
{
    float r = (float)rand();       // 0 - RAND_MAX
    float t = r / (float)RAND_MAX; // 0. - 1.

    return low + t * (high - low);
}

void findTemperature(int month)
{

    float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.); // angle of earth around the sun

    float temp = AVG_TEMP - AMP_TEMP * cos(ang);
    NowTemp = temp + Ranf(-RANDOM_TEMP, RANDOM_TEMP);
}
void findPrecipitation(int month)
{
    float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.); // angle of earth around the sun

    float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
    NowPrecip = precip + Ranf(-RANDOM_PRECIP, RANDOM_PRECIP);
    if (NowPrecip < 0.)
        NowPrecip = 0.;
}