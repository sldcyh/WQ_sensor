//updated on 3rd July 2018
// Script now modified to get data from YSI EXO2 through Modbus SOA
// Script should run on raspberryPI
// Double Check the server IP, USB ports


//updated on 26th July 2017
//New reconnection logic
// new version on 13 July
// improve robustness and more log detail to monitor the running progress

/*new version on 4 junly 2017
module to read data from AT600 with modbus
module to write data in CSV
module to send data to fiware context brokeer
module to sent data to influxdb */

// By Yiheng Chen
// Water and Environment Mangement Research Center,University of Bristol
#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <time.h>
#include <curl/curl.h>
#if LIBCURL_VERSION_NUM < 0x070c03
#error "upgrade your libcurl to no less than 7.12.3"
#endif


struct parameter_struct {
  char name[50];
  int total_reg;
  double value;
};

int post_curl_influxdb(struct parameter_struct *p, int nor) {
  // push data to influxdb
  // double check the ip address and DB name
  CURL *hnd = curl_easy_init();
  char postbuffer[500];

  curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(hnd, CURLOPT_URL, "http://192.168.0.100:8086/write?db=BIOWQ&precisio=s"); // change the IP and DB address here

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "--data-binary");
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
  for (size_t i = 0; i < nor; i++) {
    sprintf(postbuffer, "%s value=%.4f", p->name,p->value);
    p++;
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, postbuffer);
    CURLcode ret = curl_easy_perform(hnd);
    memset(postbuffer,0,sizeof(postbuffer));
  }

  return 0;
}


int patchcurl(struct parameter_struct *p, int nor) {
// function to send the data to fiware, not using this anymore
  
  CURL *hnd = curl_easy_init();
  char postbuffer[200];


  curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "PATCH");
  curl_easy_setopt(hnd, CURLOPT_URL, "http://10.20.92.105:1026/v2/entities/waterquality/attrs");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "cache-control: no-cache");
  headers = curl_slist_append(headers, "content-type: application/json");
  headers = curl_slist_append(headers, "fiware-servicepath: /waterquality");
  headers = curl_slist_append(headers, "fiware-service: uob");
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
  for (size_t i = 0; i < nor; i++) {

    sprintf(postbuffer, "{\n\t\"%s\" : \n  \t{   \t\n  \t\t\"type\": \"Float\",\n\t\t\"value\": %f\n\t}\n}\n", p->name,p->value);
    p++;
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, postbuffer);

    CURLcode ret = curl_easy_perform(hnd);
    memset(postbuffer,0,sizeof(postbuffer));
  }
  return 0;
}

void write_time(FILE *pf ){
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );

  fprintf(pf, "%d, %d, %d, %d:%d:%d, ",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

}
int check_time(){
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  if ((timeinfo->tm_min)%5==0  && (timeinfo->tm_sec==0)){
    //  printf("%d min, %d sec, check time success \n ",timeinfo->tm_min,timeinfo->tm_sec);//
    // make sure the program run every 5 mins, such as 01:00, 01:05,01:10, etc
    return 0;
  }
  else
  {
    //printf("%d min, %d sec, check time failed \n ",timeinfo->tm_min,timeinfo->tm_sec);//
    return 1;
  }

}

void write_data(FILE *pf, struct parameter_struct *p , int nor){
  for (int i = 0; i < nor; i++) {
    fprintf(pf, "%f,",p++->value);
  }
  fprintf(pf, "\n");

}

}
int main(int argc, char *argv[])
{

  modbus_t *ctx;
  uint16_t tab_reg[2];
  int rc,c=0,time_inter=300,reconnect_time=1;  /* time in sec*/
  int i,j,k;
  int YOUR_DEVICE_ID=1;
  //float temperature;

  FILE *pf_data, *pf_log,*pf_conf;
  int nor=9;                            //specify the number of parameter according to the config.txt
  struct parameter_struct parameter[9],*p; //specify the number of parameter according to the config.txt
  p=parameter;

  // used to generate the output file name
  time_t rawtime;
  struct tm * timeinfo;
  char file_name[80];

  time (&rawtime);
  timeinfo = localtime (&rawtime);

  strftime (file_name,80,"wqdata_%Y-%m-%d_%H:%M:%S.csv",timeinfo);

  pf_conf=fopen("config.txt","rt");
  if(pf_conf==NULL)
  {
    printf("\n configure file open failed \n");
    return -1;
  }

  for (i = 0; i < nor; i++) {
    fscanf(pf_conf,"%s %d",parameter[i].name, &parameter[i].total_reg);
    /*printf("%s,%d\n", parameter[i].name,parameter[i].total_reg);*/
  };
  fclose(pf_conf);
  // set up modbus connection configuration



  while (TRUE) {
    /* code */
    /*  ctx = modbus_new_rtu("/dev/ttyS0", 19200, 'E', 8, 1);*/
    //printf("Waiting to start\n");//
    while (check_time()) {
      sleep(1);     //sleep 1 sec
    }
    //start the process
    pf_log=fopen("log.txt","a");
    if(pf_log==NULL)
    {
      printf("\n log file open failed \n");
      return -1;
    }
    write_time( pf_log );
    fprintf(pf_log,"The %d time run started\n",++c );
    //printf("start reading the sensor\n");
    //ctx = modbus_new_tcp("10.20.92.120", 8899);
    ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);

    modbus_set_slave(ctx, YOUR_DEVICE_ID);
    modbus_set_response_timeout(ctx, 3, 0);

    
    if (ctx == NULL) {
      fprintf(pf_log, "Unable to create the libmodbus context\n");
      return -1;
    }
    else {
      fprintf(pf_log, "Sensor connection prepared\n");
    }

    while (modbus_connect(ctx) == -1) {
	fprintf(pf_log, "Connection failed: %s\nTry again in %d sec\n", modbus_strerror(errno),reconnect_time);
	modbus_free(ctx);
      /*return -1;*/
      sleep(reconnect_time);
      ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);

      modbus_set_slave(ctx, YOUR_DEVICE_ID);
      modbus_set_response_timeout(ctx, 3, 0);
      if (ctx == NULL) {
        fprintf(pf_log, "Unable to create the libmodbus context\n");
      return -1;
      }
      else {
        fprintf(pf_log, "Sensor connection prepared\n");
      }

    }

    fprintf(pf_log, "Sensor connected\n");

    for ( i = 0; i < nor; i++) {
      while ( (rc = modbus_read_registers(ctx, parameter[i].total_reg, 2, tab_reg)) == -1) {
        /*   fprintf(stderr, "%s\n", modbus_strerror(errno)); */
      }
      parameter[i].value = modbus_get_float_dcba(tab_reg);
      /* printf("%s=%f \n", parameter[i].name,parameter[i].value);*/
      modbus_flush(ctx);
    }

    modbus_close(ctx);
    modbus_free(ctx);
    /*Write out puf file */
    fprintf(pf_log, "start logging data to csv\n");
    pf_data=fopen(file_name,"a");
    if(pf_data==NULL)
    {
      printf("\n Ootput file open failed \n");
      return -1;
    }
    /* write output file head*/
    if (ftell(pf_data)==0) {
      fprintf(pf_data, "day,month,year,time");
      for ( i = 0; i < nor; i++) {
        fprintf(pf_data, ",%s",p++->name);
      }
      fprintf(pf_data, "\n");
    }
    write_time( pf_data );
    write_data( pf_data, parameter, nor);
    fclose(pf_data);
    fprintf(pf_log, "logging data to csv done\n");
    // fprintf(pf_log, "start post data to fiware\n");

    // patchcurl(parameter,nor); /* post data to fiware*/
    // fprintf(pf_log, "post data to fiware done\n");
    fprintf(pf_log, "start post data to influxDB\n");
    post_curl_influxdb(parameter,nor); // post data to influxdb
    fprintf(pf_log, "post data to influxDB done\n");

    /* write running log */

    write_time( pf_log );
    fprintf(pf_log,"The %d time run has completed, waiting %d sec for next run \n",c,time_inter );
    fclose(pf_log);
    /*sleep(time_inter);*/
  }
  printf("run completed \n");

  return 0;
}
