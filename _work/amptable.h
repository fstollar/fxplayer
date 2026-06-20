// AMPtable.h for F/X Player
// (C) 1997-98 by Apollo / STIGMA

#ifndef __AMPTABLE_H
#define __AMPTABLE_H

void calcVol8for16 ( char *VolPointer );
void calcVol8for32 ( char *VolPointer );

void calcMasterVol8for16 ( unsigned long MasterVolume , char *Pointer);
void calcMasterVol16for16 ( unsigned long MasterVolume , char *Pointer);
void calcMasterVolume32 ( unsigned long MasterVolume , char *Pointer);
//void MasterVol8for16 ( char *Source, char *Destin, unsigned long length);
//void MasterVol16for16 ( char *Source, char *Destin, unsigned long length);

void DoMasterVolumeCalculations ( char *Source, char *Destin, unsigned long length);

//extern char *MasterVolumeTable;
//extern char *Voltable8;

#endif
