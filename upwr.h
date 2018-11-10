typedef struct looping looping;

typedef struct adc adc;

int smart_delay(looping *looping);

void read_adc(adc *adc);

void output_serial(adc *adc);
