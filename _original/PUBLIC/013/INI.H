#include <string.h>
#include <stdio.h>

FILE *openfile(char *filetoload)
{
  FILE *file;
  file=fopen(filetoload,"rt");
  return (file);
}

char *getdata(FILE *file,char *name)
{
  char string[100];
  char string2[100];

  name=strupr(name);

  fseek(file,0,0);

  fscanf(file,"%s",&string);
  *string=*strupr(string);
  while (strnicmp(name,string,strlen(name))) {
    fscanf(file,"%s",&string);
    *string=*strupr(string);
  }

  strncpy(string2,strchr(string,'=')+1,50);
  return string2;
}

void closefile(FILE *file)
{
  fclose(file);
}

unsigned long stol(char *string,int base)
{
  int i;
  unsigned char t;
  unsigned long number, counter;

  number=0;
  counter=1;

  for (i=strlen(string)-1; i>=0; i--) {
    t=string[i];
    t-=48;
    if (t>9) t-=8;
    number+=t*counter;
    counter*=base;
  }

  return number;
}
