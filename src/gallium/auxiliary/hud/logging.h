#include "hud_private.h"

void *logging(void *empty){
	loggingOn = true;
	log_start = os_time_get();

	while (loggingOn){
		uint64_t now = os_time_get();
        elapsedLog = (now - log_start) / 1000000;
        if (elapsedLog >= duration)
			loggingOn = false;

        fprintf(outFile, "%f,%f,%f\n", currentValues.fps, currentValues.cpu, currentValues.gpu);
        fflush(outFile);

		usleep(log_period * 1000);
	}
    fclose(outFile);
    return NULL;
}