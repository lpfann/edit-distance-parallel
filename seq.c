#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "dll.h"

struct AlignJob{
	int blocksize;
	int work;
	int startCoordA;
	int rest;
	int lengthB;
	char *sequence1,*sequence2;
	struct list_head *results;
};
struct Result{
	struct list_head head;
	int startA;
	int startB;
	int score;
};

void print_results(struct list_head *list){
	
	struct list_head *start = list;
	struct Result *result = (struct Result *)list;
	struct list_head *temp = list->next;	
	
	while (temp != start)
	{
		result = (struct Result *)temp;
		printf("%i %i %i\n\r",result->startA,result->startB,result->score);
		temp = temp->next;
	}

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


int editDistanceDynamic(char *stringA, char *stringB){

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

	// Ausgabe des Edit-Scores
	return editMatrix[lenA][lenB];
}

void *scoreCalculatorThread(void *arg){
	struct AlignJob *job = arg;
	int blocksize = job->blocksize;
	char *stringA = job->sequence1;
	char *stringB = job->sequence2;
	int work = job->work;
	int lengthB = job->lengthB;
	struct list_head *resultList = malloc(sizeof(struct list_head));
	list_init(resultList);
	job->results = resultList;

	// Äußere Schleife geht StringA blockweise durch
	int i;
	for (i = 0; i < work; ++i){
		char *subA = malloc(blocksize);
		memcpy(subA, stringA+(blocksize*i),blocksize);
		// Durchgehen des zweiten Strings
		int j;
		int workB = lengthB/blocksize;
		for (j = 0; j < workB; ++j){
			char *subB = malloc(blocksize);
			memcpy(subB, stringB+(blocksize*j),blocksize);
			
			// Result Element für die Speicherung der Ergebnisse mit Koordinaten
			struct Result *res = malloc(sizeof(struct Result));
			res->score= editDistanceDynamic(subA,subB);
			res->startA= job->startCoordA + i*blocksize;
			res->startB= j*blocksize;
			if (res->score > 30){
				list_add(&(res->head),resultList);
			}
		}
		int restB;
		if (( restB = lengthB%blocksize)  > 0){
			char *subB = malloc(restB);
			memcpy(subB, stringB+(blocksize*(workB)),restB);

			struct Result *res = malloc(sizeof(struct Result));
			res->score= editDistanceDynamic(subA,subB);
			res->startA= job->startCoordA + i*blocksize;
			res->startB= workB*blocksize;
			// printf("%i %i %i %s %i\n",res->startA,res->startB ,restB,subB,workB);
			if (res->score > 30){
				list_add(&(res->head),resultList);
			}

		}
	}

	print_results(resultList);
	return NULL;
}


char* importSequence(char *argv){
	FILE *sequenceFile;
	char *filename = argv;
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
	int blocksize = 25;
	int threadNumber  = 4;
	pthread_t threads[threadNumber];

	char *seqA = importSequence(argv[1]);
	char *seqB = importSequence(argv[2]);
	int lengthA = strlen(seqA);
	int lengthB = strlen(seqB);

	int blockCountSeqA = lengthA/blocksize;
	int restA = lengthA%blocksize;
	int blockCountSeqB = strlen(seqB)/blocksize;
	int restB = lengthB%blocksize;
	printf("SequenzA:  %i Blöcke, SequenzB: %i Blöcke, RestA %i ,RestB %i \n",blockCountSeqA,blockCountSeqB,restA,restB);

	if (blockCountSeqA>=blockCountSeqB){
		int workPerThread = blockCountSeqA/threadNumber;
		if (workPerThread == 0) threadNumber=1; // Falls Sequenzen zu kurz für Multithreading sind
		int i;
		printf("Threads werden erstellt:\n");
		for (i = 0; i < threadNumber; ++i){
			char *seqAforThread;
			char *seqBforThread;
			struct AlignJob *job = malloc(sizeof(struct AlignJob));

			printf("Thread Nr: %i wird erstellt\n",i);
			if (i == threadNumber-1){
				// Der letzte Thread berechnet den Modulo Teil mit
				workPerThread += blockCountSeqA % threadNumber;
				if (workPerThread == 0) workPerThread = 1;
				seqAforThread = malloc(sizeof(char)*(workPerThread*blocksize+restA));
				memcpy( seqAforThread, seqA+(i * blocksize * workPerThread),blocksize*workPerThread+restA);
				seqAforThread[blocksize*workPerThread+restA] = '\0';
				job->rest = restA;	
			}else {
				// Normale Aufteilung der Sequenz A für den Thread
				seqAforThread = malloc(sizeof(char)*workPerThread*blocksize);
				memcpy( seqAforThread, seqA+(i * blocksize * workPerThread),blocksize*workPerThread);
				seqAforThread[blocksize*workPerThread] = '\0';
			}
			// Kopieren von SequenzB
			seqBforThread = malloc(lengthB);
			memcpy( seqBforThread, seqB,lengthB);
			// Job erstellen
			job->sequence1 = seqAforThread;
			job->sequence2 = seqBforThread;
			job->blocksize= blocksize;
			job->lengthB = lengthB;
			job->work = workPerThread;
			job->startCoordA = i * blocksize * workPerThread; 
			pthread_create(&threads[i], NULL, scoreCalculatorThread, job);
		}

		for (i = 0; i < threadNumber; ++i){
			pthread_join(threads[i], NULL);
		}	
	} else {
		printf("Die erste Sequenz muss die längere sein!\n" );
	}

	// char *sequence = importSequence(argv);
	// if (sequence){
	// 	// printf("%s",sequence);
	// }


return 0;
}
