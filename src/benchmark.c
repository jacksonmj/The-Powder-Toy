#include <stdio.h>
#include "air.h"
#include "powder.h"
#include "gravity.h"
#include "powdergraphics.h"
#include "benchmark.h"
#include "save.h"
#include <math.h>
#include "simulation/Simulation.h"

char *benchmark_file = NULL;
double benchmark_loops_multiply = 1.0; // Increase for more accurate results (particularly on fast computers)
int benchmark_repeat_count = 5; // this too, but try benchmark_loops_multiply first

double benchmark_get_time()
{
	return SDL_GetTicks()/1000.0;
}

// repeat_count - how many times to run the test, iterations_count = number of loops to execute each time

#define BENCHMARK_INIT(repeat_count, iterations_count) \
{\
	int bench_i, bench_iterations = (iterations_count)*benchmark_loops_multiply;\
	int bench_run_i, bench_runs=(repeat_count);\
	double bench_mean=0.0, bench_prevmean, bench_variance=0.0;\
	double bench_start, bench_end;\
	for (bench_run_i=1; bench_run_i<=bench_runs; bench_run_i++)\
	{

#define BENCHMARK_RUN() \
		bench_start = benchmark_get_time();\
		for (bench_i=0;bench_i<bench_iterations;bench_i++)\
		{


#define BENCHMARK_START(repeat_count, iterations_count) \
	BENCHMARK_INIT(repeat_count, iterations_count) \
	BENCHMARK_RUN()

#define BENCHMARK_END() \
		}\
		bench_end = benchmark_get_time();\
		bench_prevmean = bench_mean;\
		bench_mean += ((double)(bench_end-bench_start) - bench_mean) / bench_run_i;\
		bench_variance += (bench_end-bench_start - bench_prevmean) * (bench_end-bench_start - bench_mean);\
	}\
	if (bench_runs>1) \
		printf("mean time per iteration %g ms, std dev %g ms (%g%%)\n", bench_mean/bench_iterations * 1000.0, sqrt(bench_variance/(bench_runs-1))/bench_iterations * 1000.0, sqrt(bench_variance/(bench_runs-1)) / bench_mean * 100.0);\
	else \
		printf("mean time per iteration %g ms\n", bench_mean/bench_iterations * 1000.0);\
}


void benchmark_run()
{
	pixel *vid_buf = (pixel*)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	if (benchmark_file)
	{
		int size;
		char *file_data;
		file_data = (char*)file_load(benchmark_file, &size);
		if (file_data)
		{
			if(!parse_save(file_data, size, 1, 0, 0, bmap, fvx, fvy, vx, vy, pv, signs, parts, pmap))
			{
				printf("Save speed test:\n");

				printf("Update particles+air: ");
				BENCHMARK_INIT(benchmark_repeat_count, 200)
				{
					parse_save(file_data, size, 1, 0, 0, bmap, fvx, fvy, vx, vy, pv, signs, parts, pmap);
					sys_pause = framerender = 0;
					BENCHMARK_RUN()
					{
						update_air();
						if(aheat_enable)
							update_airh();
						globalSim->UpdateParticles();
					}
				}
				BENCHMARK_END()

				printf("Load save: ");
				BENCHMARK_INIT(benchmark_repeat_count, 100)
				{
					BENCHMARK_RUN()
					{
						parse_save(file_data, size, 1, 0, 0, bmap, fvx, fvy, vx, vy, pv, signs, parts, pmap);
					}
				}
				BENCHMARK_END()

				printf("Update particles - paused: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1000)
				{
					parse_save(file_data, size, 1, 0, 0, bmap, fvx, fvy, vx, vy, pv, signs, parts, pmap);
					sys_pause = 1;
					framerender = 0;
					BENCHMARK_RUN()
					{
						globalSim->UpdateParticles();
					}
				}
				BENCHMARK_END()

				printf("Update particles - unpaused: ");
				BENCHMARK_INIT(benchmark_repeat_count, 200)
				{
					parse_save(file_data, size, 1, 0, 0, bmap, fvx, fvy, vx, vy, pv, signs, parts, pmap);
					sys_pause = framerender = 0;
					BENCHMARK_RUN()
					{
						globalSim->UpdateParticles();
					}
				}
				BENCHMARK_END()

				printf("Render particles: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1500)
				{
					parse_save(file_data, size, 1, 0, 0, bmap, fvx, fvy, vx, vy, pv, signs, parts, pmap);
					sys_pause = framerender = 0;
					display_mode = 0;
					render_mode = RENDER_BASC;
					decorations_enable = 1;
					globalSim->UpdateParticles();
					BENCHMARK_RUN()
					{
						render_parts(vid_buf);
					}
				}
				BENCHMARK_END()

				printf("Render particles+fire: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1200)
				{
					parse_save(file_data, size, 1, 0, 0, bmap, fvx, fvy, vx, vy, pv, signs, parts, pmap);
					sys_pause = framerender = 0;
					display_mode = 0;
					render_mode = RENDER_FIRE;
					decorations_enable = 1;
					globalSim->UpdateParticles();
					BENCHMARK_RUN()
					{
						render_parts(vid_buf);
						render_fire(vid_buf);
					}
				}
				BENCHMARK_END()


			}
			free(file_data);
		}
	}
	else
	{
		printf("General speed test:\n");
		clear_sim();

		gravity_init();
		update_grav();
		printf("Gravity - no gravmap changes: ");
		BENCHMARK_START(benchmark_repeat_count, 100000)
		{
			update_grav();
		}
		BENCHMARK_END()

		printf("Gravity - 1 gravmap cell changed: ");
		BENCHMARK_START(benchmark_repeat_count, 1000)
		{
			th_gravmap[(YRES/CELL-1)*(XRES/CELL) + XRES/CELL - 1] = (bench_i%5)+1.0f; //bench_i defined in BENCHMARK_START macro
			update_grav();
		}
		BENCHMARK_END()

		printf("Gravity - membwand: ");
		BENCHMARK_START(benchmark_repeat_count, 10000)
		{
			membwand(gravy, gravmask, (XRES/CELL)*(YRES/CELL)*sizeof(float), (XRES/CELL)*(YRES/CELL)*sizeof(unsigned));
			membwand(gravx, gravmask, (XRES/CELL)*(YRES/CELL)*sizeof(float), (XRES/CELL)*(YRES/CELL)*sizeof(unsigned));
		}
		BENCHMARK_END()

		printf("Air - no walls, no changes: ");
		BENCHMARK_START(benchmark_repeat_count, 3000)
		{
			update_air();
		}
		BENCHMARK_END()

		printf("Air + aheat - no walls, no changes: ");
		BENCHMARK_START(benchmark_repeat_count, 1600)
		{
			update_air();
			update_airh();
		}
		BENCHMARK_END()
	}
	free(vid_buf);
}


