
char *getdata(FILE *file,char *name)
{
  char string[100];
  char string2[100];

  //name=strupr(name);

  fseek(file,0L,0);

  fscanf(file,"%s",&string);
  //*string=*strupr(string);
  while (strnicmp(name,string,strlen(name)))
  {
    fscanf(file,"%s",&string);
    //*string=*strupr(string);
  }

  strncpy(string2,strchr(string,'=')+1,50);
  return string2;
}

