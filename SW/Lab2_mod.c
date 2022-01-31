#define REALTIMEFREQ 1024 
#define REALTIMEPERIOD (80000000/REALTIMEFREQ)
#define ADCSAMPFREQ 125
#define ADCSAMPPERIOD (80000000/ADCSAMPFREQ)
volatile uint32_t RealTimeCount;

// Runs from Timer2A ISR
void RealTimeTask(void) { // executes at 1024 Hz
    PF3 ^= 0x08;
    PF3 ^= 0x08;
    RealTimeCount++;
    Jitter_Measure();
    PF3 ^= 0x08;
}

void Pause(void) {
    LaunchPad_WaitForTouch();
    LaunchPad_WaitForRelease();
}

// Runs from Timer0A ISR
void RealTimeSampling(void) { // runs at 125 Hz
    uint32_t ADCvalue;
    PF2 ^= 0x04;                // profile
    PF2 ^= 0x04;                // profile
    ADCvalue = ADC0_InSeq3();
    Dump_Capture(ADCvalue);
    PF2 ^= 0x04;                // profile
}

int main0(void) {
    // main0, run TExaS oscilloscope
    // analog scope on PD3, PD2, PE2 or PB5 using ADC1
    DisableInterrupts();
        TExaS_Init(SCOPE); // connect analog input to PD3
    LaunchPad_Init();
    uint32_t count = 0;
    EnableInterrupts();
    while(1){
        count++;
        if(count > 10000000){
            PF1 ^= 0x02;  // toggle slowly
            count = 0;
        }
    }
}

uint32_t Min,Max,Jitter;
uint32_t Histogram[64]; // probability mass function
uint32_t Center;
#define FIXED 100
uint32_t Sigma; // units 1/FIXED
uint32_t YY; //GlobalVariable
int main1(void) {
    int line = 1;
    // main1, study jitter with real logic analyzer
    DisableInterrupts();
    YY = 0;
    PLL_Init(Bus80MHz);  // 80 MHz
    ST7735_InitR(INITR_REDTAB);
    ST7735_SetTextColor(ST7735_YELLOW);
    Dump_Init();
    LaunchPad_Init();
    Timer2A_Init(&RealTimeTask,REALTIMEPERIOD,2);  // PF3 toggle at about 1kHz  
    ST7735_FillScreen(0);  // set screen to black
    ST7735_SetCursor(0,0);
    ST7735_OutString("Lab 2 Jitter, 12.5ns");
    while(1){
        DisableInterrupts();
        Jitter_Init(); // analyze 1 kHz real time task
        RealTimeCount = 0;
        EnableInterrupts();
        while(RealTimeCount < 3000){ // 3 second measurement
            PF1 ^= 0x02;  // toggles when running in main
            YY = (YY*12345678)/1234567;  // the divide instruction causes jitter
        }
        ST7735_SetCursor(0,line);
        ST7735_OutString("Jitter =           ");
        ST7735_SetCursor(9,line);
        ST7735_OutUDec(Jitter_Get());
        line = (line+1)%15;
    }
}
int main2(void){int line=1;
  // main2, study jitter without real logic analyzer, using TExaS
	// Run TExaSdisplay on the PC in logic analyzer mode
  DisableInterrupts();
  YY = 0;
	TExaS_Init(LOGICANALYZERF);
  ST7735_InitR(INITR_REDTAB);
  ST7735_SetTextColor(ST7735_YELLOW);
  Dump_Init();
  LaunchPad_Init();
  Timer2A_Init(&RealTimeTask,REALTIMEPERIOD,2);  // PF3 toggle at about 1kHz  
  ST7735_FillScreen(0);  // set screen to black
  ST7735_SetCursor(0,0);
  ST7735_OutString("Lab 2 Jitter, 12.5ns");
  while(1){
    DisableInterrupts();
    Jitter_Init(); // analyze 1 kHz real time task
    RealTimeCount = 0;
    EnableInterrupts();
    while(RealTimeCount < 3000){ // 3 second measurement
      PF1 ^= 0x02;  // toggles when running in main
      YY = (YY*12345678)/1234567;  // the divide instruction causes jitter
    }
    ST7735_SetCursor(0,line);
    ST7735_OutString("Jitter =           ");
    ST7735_SetCursor(9,line);
    ST7735_OutUDec(Jitter_Get());
    line = (line+1)%15;
  }
}

// decimal fixed point 1/FIXED

uint32_t StandardDeviation(uint32_t buffer[], uint32_t size){
  int32_t sum = 0;
  for(int i=0;i<size;i++){
    sum = sum+FIXED*buffer[i];
  }
  int32_t ave = sum/size;
  sum = 0;
  for(int i=0;i<size;i++){
    sum = sum+(FIXED*buffer[i]-ave)*(FIXED*buffer[i]-ave);
  }
  return sqrt(sum/(size-1)); // units 1/FIXED
}
void PrintStandardDeviation(uint32_t sigma){
	ST7735_SetCursor(0,3);
  ST7735_OutString("s = ");
  ST7735_OutUDec(sigma/FIXED);
	ST7735_OutChar('.');
	sigma = sigma%FIXED;
	uint32_t n=FIXED/10;
	while(n){
		ST7735_OutUDec(sigma/n);
    sigma = sigma%n;
		n = n/10;
	}
}
int main(void){uint32_t i,d,data,time,sac; 
  // main3, study jitter, CLT with real logic analyzer
  uint32_t* DataBuf;
  uint32_t* TimeBuf;
  PLL_Init(Bus80MHz);                   // 80 MHz
  ST7735_InitR(INITR_REDTAB);
  ST7735_SetTextColor(ST7735_YELLOW);
  Dump_Init();
  DataBuf = Dump_Data();
  TimeBuf = Dump_Time();
  LaunchPad_Init();
  Timer2A_Init(&RealTimeTask,REALTIMEPERIOD,2);  // PF3 toggle at about 1kHz  
  ADC0_InitSWTriggerSeq3(1); // Feel free to use any analog channel
  Timer0A_Init(&RealTimeSampling,ADCSAMPPERIOD,1);         // set up Timer0A for 100 Hz interrupts
  sac = 0;
  ST7735_OutString("Lab 2 PMF, SAC="); ST7735_OutUDec(sac);
  while(1){
		DisableInterrupts();
    ADC0_SAC_R = sac;
    Dump_Init();
    Jitter_Init(); // analyze 1 kHz real time task
    RealTimeCount = 0;
    EnableInterrupts();
    while(Dump_Count() < DUMPBUFSIZE){
      PF1 ^= 0x02;       // Heartbeat
		} // wait for buffers to fill
    ST7735_FillScreen(0);  // set screen to black
    ST7735_SetCursor(0,0);
    ST7735_OutString("Lab 2 PMF, SAC="); ST7735_OutUDec(sac);
    for(i=0; i<64; i++) Histogram[i] = 0; // clear
    Center = DataBuf[0];
    for(i = 0; i < DUMPBUFSIZE; i++){
      data = DataBuf[i];
      if(data<Center-32){
         Histogram[0]++;
      }else if(data>=Center+32){
         Histogram[63]++;
      }else{
        d = data-Center+32;
        Histogram[d]++;
      }
    }
    Min = 0xFFFFFFFF; 
		Max = 0;
    for(i = 1; i < DUMPBUFSIZE; i++){
      time = TimeBuf[i]-TimeBuf[i-1]; // elapsed
      if(time < Min) Min = time;
      if(time > Max) Max = time;
    }
    Jitter = Max - Min;
    Sigma = StandardDeviation(DataBuf,DUMPBUFSIZE);
    ST7735_SetCursor(0,1);
    ST7735_OutString("ADC Jitter = ");
	  ST7735_OutUDec(Jitter);
    ST7735_SetCursor(0,2);
    ST7735_OutString("PF3 Jitter = ");
    ST7735_OutUDec(Jitter_Get());
    ST7735_PlotClear(0,DUMPBUFSIZE/2);
    for(i=0; i<63; i++){
      if(Histogram[i]>=DUMPBUFSIZE/2) Histogram[i]=(DUMPBUFSIZE/2)-1;
      ST7735_PlotBar(Histogram[i]);
      ST7735_PlotNext();
      ST7735_PlotBar(Histogram[i]);
      ST7735_PlotNext();
    }
		PrintStandardDeviation(Sigma);
    Pause();
    if(sac<6) sac++;
    else sac = 0;  
  }
}

int main4(void){uint32_t i,d,data,time,sac; 
  // main4, study jitter, CLT without real logic analyzer, using TExaS
	// Run TExaSdisplay on the PC in logic analyzer mode
  uint32_t* DataBuf;
  uint32_t* TimeBuf;
	TExaS_Init(LOGICANALYZERF);
  ST7735_InitR(INITR_REDTAB);
  ST7735_SetTextColor(ST7735_YELLOW);
  Dump_Init();
  DataBuf = Dump_Data();
  TimeBuf = Dump_Time();
  LaunchPad_Init();
  Timer2A_Init(&RealTimeTask,REALTIMEPERIOD,2);  // PF3 toggle at about 1kHz  
  ADC0_InitSWTriggerSeq3(1); // Feel free to use any analog channel
  Timer0A_Init(&RealTimeSampling,ADCSAMPPERIOD,1);         // set up Timer0A for 100 Hz interrupts
  sac = 0;
  ST7735_OutString("Lab 2 PMF, SAC="); ST7735_OutUDec(sac);
  while(1){
		DisableInterrupts();
    ADC0_SAC_R = sac;
    Dump_Init();
    Jitter_Init(); // analyze 1 kHz real time task
    RealTimeCount = 0;
    EnableInterrupts();
    while(Dump_Count() < DUMPBUFSIZE){
      PF1 ^= 0x02;       // Heartbeat
		} // wait for buffers to fill
    ST7735_FillScreen(0);  // set screen to black
    ST7735_SetCursor(0,0);
    ST7735_OutString("Lab 2 PMF, SAC="); ST7735_OutUDec(sac);
    for(i=0; i<64; i++) Histogram[i] = 0; // clear
    Center = DataBuf[0];
    for(i = 0; i < DUMPBUFSIZE; i++){
      data = DataBuf[i];
      if(data<Center-32){
         Histogram[0]++;
      }else if(data>=Center+32){
         Histogram[63]++;
      }else{
        d = data-Center+32;
        Histogram[d]++;
      }
    }
    Min = 0xFFFFFFFF; 
		Max = 0;
    for(i = 1; i < DUMPBUFSIZE; i++){
      time = TimeBuf[i]-TimeBuf[i-1]; // elapsed
      if(time < Min) Min = time;
      if(time > Max) Max = time;
    }
    Jitter = Max - Min;
    Sigma = StandardDeviation(DataBuf,DUMPBUFSIZE);
    ST7735_SetCursor(0,1);
    ST7735_OutString("ADC Jitter = ");
	  ST7735_OutUDec(Jitter);
    ST7735_SetCursor(0,2);
    ST7735_OutString("PF3 Jitter = ");
    ST7735_OutUDec(Jitter_Get());
    ST7735_PlotClear(0,DUMPBUFSIZE/2);
    for(i=0; i<63; i++){
      if(Histogram[i]>=DUMPBUFSIZE/2) Histogram[i]=(DUMPBUFSIZE/2)-1;
      ST7735_PlotBar(Histogram[i]);
      ST7735_PlotNext();
      ST7735_PlotBar(Histogram[i]);
      ST7735_PlotNext();
    }
		PrintStandardDeviation(Sigma);
    Pause();
    if(sac<6) sac++;
    else sac = 0;  
  }
}