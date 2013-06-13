#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct AlignJob{
	pthread_t thread_id;
	char *sequence1,*sequence2;
	int editScore;
};

void sequenceVerarbeitung(char *s) {
		
    	// Zweiten temporären Zeiger
    	char *head = s;
    	while(*s != '\0') {
    		// Zeichen die gelöscht werden sollen
    		if(*s != '\r' && *s != '\n' && *s != 'a' && *s != 'c'
    			&& *s != 't' && *s != 'g' && *s != 'N') {
    			*head++ = *s++;
    		} else {
    			// Überspringen des Zeichens
    			++s;
    		}
    	}
    	// Dateiende setzen
    	*head = '\0';
}


void *editDistanceDynamic(void *arg){
	struct AlignJob *job = arg;
	char *stringA = job->sequence1;
	char *stringB = job->sequence2;

	int lenA = strlen(stringA);
	int lenB = strlen(stringB);
	int editMatrix[lenA+1][lenB+1];
	int i,j;
	for ( i = 0; i <= lenA; ++i){
		for ( j = 0; j <= lenB; ++j){
			// Erste Zeile und Spalte mit 0 Initalisieren
			if (j == 0 || i == 0){
			editMatrix[i][j] = 0;
			continue;	
			}  
			// Berechnung der Übergangsscores
			int left = editMatrix[i][j-1]-1;
			int up = editMatrix[i-1][j]-1;
			int diag;
			if (stringA[i-1] == stringB[j-1]){
			 	diag = editMatrix[i-1][j-1]+2;
			} else {
			 	diag = editMatrix[i-1][j-1]-1;
			}
			// Maximierung des Scores
			if (left >= up && left >= diag) editMatrix[i][j] = left;
			if (up >= left && up >= diag) editMatrix[i][j] = up;
			if (diag >= left && diag >= up) editMatrix[i][j] = diag;
			if (editMatrix[i][j] < 0){
				editMatrix[i][j] = 0;
			}

		}
	}
	printf("%s\n", "fertig");
	job->editScore = editMatrix[lenA][lenB];
	// Ausgabe des Edit-Scores
	return NULL;
}


char* importSequence(char *argv[]){
FILE *sequenceFile;
char *filename = argv[1];
sequenceFile = fopen(filename,"r");

	if (sequenceFile){
		printf("Datei wurde eingelesen\n");
		// Dateiende finden
		fseek(sequenceFile ,0 , SEEK_END);
		// Größe der Datei
		int size = ftell(sequenceFile);
		printf("Die Datei ist %i Zeichen groß\n",size);
		// LinePointer zurücksetzten
		rewind(sequenceFile);
		
		char *sequence = malloc(size*sizeof(char));
		// Blockweise Dateiinhalt einlesen
		fread(sequence, sizeof(char), size, sequenceFile);
		printf("Dateiinhalt wurde eingelesen\n");
		fclose(sequenceFile);

		sequence= (strstr(sequence,"\n")+1); // Erste Zeile überspringen
		sequenceVerarbeitung(sequence); // Trennzeichen und Repeats löschen
		return sequence;	
	}

printf("Fehler beim Einlesen\n");
return NULL;
}


int main (int argc, char *argv[]){
	int blocksize = 15;
	int threadNumber __attribute__ ((unused)) = 2;
	char* seq1 = "ACACACTAAAAAAAAAAAAAAAAAAABBAAA";
	char* seq2 = "AGCACACABBBBBBBBBBBBBBBBBBBBBB";

	int blockNumber1 __attribute__ ((unused))= strlen(seq1)/blocksize;
	int blocknumber2 __attribute__ ((unused))= strlen(seq2)/blocksize;

	printf("%i %i\n",blockNumber1, blocknumber2 );
	// char *sequence = importSequence(argv);
	// if (sequence){
	// 	// printf("%s",sequence);
	// }
	
	struct AlignJob *worker = malloc(sizeof(struct AlignJob));
	worker->sequence1 = "ACACACTAAAAAAAAAAAAAAAAAAABBAA";
	worker->sequence2 = "AGCACACABBBBBBBBBBBBBBBBBBBBBB";
	pthread_create(&(worker->thread_id), NULL, editDistanceDynamic, worker);
	pthread_join(worker->thread_id, NULL);
	// int dist = editDistanceDynamic("ACACACTA","AGCACACA");
	printf("%i\n",worker->editScore );

return 0;
}
