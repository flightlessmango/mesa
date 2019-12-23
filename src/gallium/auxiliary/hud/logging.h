#include "hud_private.h"

void *logging(void *empty){
    remove("/tmp/mango-gallium");
    outFile = fopen("/tmp/mango-gallium", "a");
    duration_string = getenv("LOG_DURATION");
    if (duration_string != NULL)
      duration = atoi(duration_string);

    int period = 500;
    int arrayLength = duration * (1000 / period) + 1;
    
    struct logData *logArray = malloc(arrayLength*sizeof(struct logData));
	loggingOn = true;
	uint64_t start = os_time_get();
    
	while (loggingOn){
		uint64_t now = os_time_get();
        elapsedLog = (now - start) / 1000000;
        if (elapsedLog >= duration)
			loggingOn = false;

        fprintf(outFile, "%f,%f,%f\n", currentValues.fps, currentValues.cpu, currentValues.gpu);
        fflush(outFile);

		usleep(period * 1000);
	}
    fclose(outFile);
    return NULL;
}