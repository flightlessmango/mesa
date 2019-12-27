#include "hud_private.h"

void *logging(void *empty){
	loggingOn = true;
	log_start = os_time_get();

	while (loggingOn){
		uint64_t now = os_time_get();
        elapsedLog = (now - log_start) / 1000000;
        int elapsed = now - log_start;
        if (elapsedLog >= duration)
			loggingOn = false;

        fprintf(outFile, "%f,%f,%f,%i\n", currentValues.fps, currentValues.cpu, currentValues.gpu, elapsed);
        fflush(outFile);

		usleep(log_period * 1000);
	}
    fclose(outFile);
    return NULL;
}