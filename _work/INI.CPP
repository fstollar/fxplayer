//	INIT-Datei auswerten
//
//	Schluesselwoerter:	"[xyz]"
//  Parameter:					"Param:"
//

#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <stdlib.h>



int getline (char *line, int max, FILE *pointer);
int strmid(char *destination, char *source, int offset, int len);
int Get_Key(char *Zeile, char *Param);
void Auswertung(int Key, char *Param);



FILE *fp;
int maxchars = 199;
char Zeile[200];

char INIT_FILE[] = "INIT.DAT\0";

#define Key1_Param1		101
#define Key1_Param2		102
#define Key2_Param1		201
#define Key2_Param2		202


char *Key_Word[] = {	"Key1", "Key2", "*" };

char *Param_Word[] = { "Adr" , "Vol" , "Path" , "*" };



int main(int argc, char *argv[])
{
	int len, Key;
	char *Param, Par[100];

	Param = Par;



	if ((fp = fopen(INIT_FILE, "r")) == NULL)
	{
		printf ("Kann Datei %s nicht ”ffnen \n",INIT_FILE);
		return 1;
	}


	while ((len = getline(Zeile, maxchars, fp)) != 0)
	{

		Key = Get_Key(Zeile, Param);

		if (Key) Auswertung(Key, Param);

	}

fclose (fp);
return 0;
}


// ******************************************************************
// **
// **	Schluessel = Get_Key (INPUT Zeile, OUTPUT Parameter)
// ** ---------------------------------------------------------------
// ** liest aus der Zeile Schluesselwoerter bzw. Parameter und
// ** gibt als Returnwert einen Schluessel zurueck mit dessen Hilfe
// ** eine eindeutige Zuordnung getroffen werden kann.
// ** Parameter liefert den uebergebenen Parameter als String
// **
// ******************************************************************

int Get_Key(char *Zeile, char *Param)
{
  char *Kennz1, *Kennz2, Teilstr[50];
 int i, flag;
 static int Key = 0;               // akt. Schluesselwort merken

 strcpy (Param , "\0" );

  if ((Kennz1 = strchr(Zeile, '[')) != NULL)
  {
   if ((Kennz2 = strchr(Zeile, ']')) != NULL)
   {
    strmid(Teilstr, Zeile, Kennz1-Zeile+1, (Kennz2-Kennz1)-1);			
    printf("Key: %s\n", Teilstr);
   }
   else
   {
    printf("']' missing in File %s\n", INIT_FILE);
    Key = 0;
    return 0;
   }
   i = 0;

   do
    {
     flag = strcmp(Teilstr, Key_Word[i]);
     i++;
    } while ((flag != 0) && (Key_Word[i][0] != '*'));


		if (flag != 0)
		{
			printf("Unbekanntes Schluesselwort: '%s'\n", Teilstr);
			Key = 0;
			return 0;
		}
		else
		{
			printf("Schluesselwort gefunden: %s\n", Teilstr);
			Key = 100*i;
			return 0;
		}
	}
	else
	{
		if ((Kennz1 = strchr(Zeile, ':')) != NULL)
		{
			strmid(Teilstr, Zeile, 0, Kennz1-Zeile);
		
			i = 0;
			do
			{
				flag = strcmp(Teilstr, Param_Word[i]);
				i++;
			}while ((flag != 0) && (Param_Word[i][0] != '*'));

			if (flag != 0) 
			{
				printf("Unbekannter Parameter: '%s'\n", Teilstr);
				return 0;
			}
			else
			{
				printf("Parameter gefunden: %s", Zeile);
				Kennz1++;
				strcpy (Param , Kennz1); // Parameter uebergeben
				if (Key == 0) i = 0;
				return (Key + i);
			}
		}
	}


return 0;
}

// ******************************************************************
// ** 
// **	Auswertung (INPUT Schluessel , INPUT Parameter)
// ** ---------------------------------------------------------------
// ** wertet anhand des Schluessels die in der INIT-Datei uebergebenen
// ** Parameter aus.
// ** INPUT Parameter enthaelt den uebergebenen Parameter als String
// **
// ******************************************************************
void Auswertung(int Key, char *Param)
{
	int Wert, i;

	printf("Param: %s", Param);

	switch (Key)
	{
		case Key1_Param1:	printf("Auswertung 101\n");
											sscanf(Param, "%d", &Wert);
											printf("Wert: %d\n\n", Wert);
											break;
		case Key1_Param2: printf("Auswertung 102\n\n");
											break;
		case Key2_Param1:	printf("Auswertung 201\n\n");
											break;
		case Key2_Param2: printf("Auswertung 202\n\n");
											break;

		default:	i = Key / 100;
							printf("Parameter unter '%s' nicht moeglich !\n", Key_Word[i-1]);
							break;
	}
}




// ** liest eine Zeile aus der Datei
int getline (char *line, int max, FILE *pointer)
{
	if (fgets(line, max, pointer) == NULL)
	   return 0;
	else
	   return strlen(line);
}

// ** liefert einen Stringausschnitt
int strmid(char *destination, char *source, int offset, int len)
{
  int i;

  source+=offset;
  for (i=1; i<=len; i++)
  {
     *destination++ = *source++;
  }
  *destination++ = '\0';
return 0;
}



