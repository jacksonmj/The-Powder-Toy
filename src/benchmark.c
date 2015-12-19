#include <stdio.h>
#include "powder.h"
#include "gravity.h"
#include "powdergraphics.h"
#include "benchmark.h"
#include "save.h"
#include <math.h>
#include "simulation/Simulation.h"
#include "simulation/air/SimAir.hpp"
#include "common/Threading.hpp"

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
			if(!parse_save(file_data, size, 1, 0, 0, globalSim->walls.getDataPtr(), globalSim->signs, parts))
			{
				printf("Save speed test:\n");

				printf("Update particles+air: ");
				BENCHMARK_INIT(benchmark_repeat_count, 200)
				{
					parse_save(file_data, size, 1, 0, 0, globalSim->walls.getDataPtr(), globalSim->signs, parts);
					sys_pause = framerender = 0;
					BENCHMARK_RUN()
					{
						globalSim->air.simulate_sync();
						globalSim->UpdateParticles();
					}
				}
				BENCHMARK_END()

				printf("Load save: ");
				BENCHMARK_INIT(benchmark_repeat_count, 100)
				{
					BENCHMARK_RUN()
					{
						parse_save(file_data, size, 1, 0, 0, globalSim->walls.getDataPtr(), globalSim->signs, parts);
					}
				}
				BENCHMARK_END()

				printf("Update particles - paused: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1000)
				{
					parse_save(file_data, size, 1, 0, 0, globalSim->walls.getDataPtr(), globalSim->signs, parts);
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
					parse_save(file_data, size, 1, 0, 0, globalSim->walls.getDataPtr(), globalSim->signs, parts);
					sys_pause = framerender = 0;
					BENCHMARK_RUN()
					{
						globalSim->UpdateParticles();
					}
				}
				BENCHMARK_END()

				printf("Draw walls: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1500)
				{
					parse_save(file_data, size, 1, 0, 0, globalSim->walls.getDataPtr(), globalSim->signs, parts);
					sys_pause = framerender = 0;
					display_mode = 0;
					render_mode = RENDER_BASC;
					decorations_enable = 1;
					// Simulate 1 second @ 60 fps to allow things to generate air, since air is not saved
					for (int i=0; i<60; i++)
						globalSim->UpdateParticles();
					BENCHMARK_RUN()
					{
						draw_walls(vid_buf);
					}
				}
				BENCHMARK_END()

				printf("Render particles: ");
				BENCHMARK_INIT(benchmark_repeat_count, 1500)
				{
					parse_save(file_data, size, 1, 0, 0, globalSim->walls.getDataPtr(), globalSim->signs, parts);
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
					parse_save(file_data, size, 1, 0, 0, globalSim->walls.getDataPtr(), globalSim->signs, parts);
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
		SDL_Delay(1000);

		CellsUChar c;
		printf("CellsData_count_and8: ");
		BENCHMARK_START(benchmark_repeat_count, 10000)
		{
			CellsData_count_and8(c);
		}
		BENCHMARK_END()
		printf("CellsData_subtract_sat: ");
		BENCHMARK_START(benchmark_repeat_count, 10000)
		{
			CellsData_subtract_sat(c, c, 1);
		}
		BENCHMARK_END()

		CellsFloat f;
		printf("CellsData_limit: ");
		BENCHMARK_START(benchmark_repeat_count, 10000)
		{
			CellsData_limit(f, -256.0f, 256.0f);
		}
		BENCHMARK_END()

		printf("update_wallmaps: ");
		BENCHMARK_START(benchmark_repeat_count, 10000)
		{
			globalSim->walls.simBeforeUpdate();
			globalSim->air.initBlockingData();
		}
		BENCHMARK_END()

		printf("Air (inside sim): ");
		BENCHMARK_START(benchmark_repeat_count, 3000)
		{
			globalSim->air.simulate_sync();
		}
		BENCHMARK_END()

		{
			auto as0 = AirSimulator_sync::create(0);
			auto as1 = AirSimulator_sync::create(1);
			AirData od, nd;
			AirSimulator_params_roInput params;
			params.wallsData = globalSim->walls.getDataPtr();
			params.prevData = od;
			params.inputData = od;
			params.outputData = nd;
			CellsUChar bmap_block;
			CellsData_fill<unsigned char>(bmap_block, 0);
			params.blockair = bmap_block;
			params.blockairh = bmap_block;

			printf("New air: ");
			params.ambientHeatEnabled = false;
			BENCHMARK_START(benchmark_repeat_count, 3000)
			{
				as1->simulate(params);
			}
			BENCHMARK_END()

			printf("New air + ambient heat: ");
			params.ambientHeatEnabled = true;
			BENCHMARK_START(benchmark_repeat_count, 3000)
			{
				as1->simulate(params);
			}
			BENCHMARK_END()

			printf("Old air: ");
			params.ambientHeatEnabled = false;
			BENCHMARK_START(benchmark_repeat_count, 3000)
			{
				as0->simulate(params);
			}
			BENCHMARK_END()

			printf("Old air + ambient heat: ");
			params.ambientHeatEnabled = true;
			BENCHMARK_START(benchmark_repeat_count, 3000)
			{
				as0->simulate(params);
			}
			BENCHMARK_END()
		}

		printf("render_fire: ");
		BENCHMARK_INIT(benchmark_repeat_count, 50)
		{
			int j, i;
			for (j=YRES/CELL/4; j<YRES/CELL-YRES/CELL/4; j++)
				for (i=XRES/CELL/4; i<XRES/CELL-XRES/CELL/4; i++)
				{
					fire_r[j][i] = 255;
					fire_g[j][i] = 255;
					fire_b[j][i] = 255;
				}
			BENCHMARK_RUN()
			{
				render_fire(vid_buf);
			}
		}
		BENCHMARK_END()

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
	}
	free(vid_buf);
}

