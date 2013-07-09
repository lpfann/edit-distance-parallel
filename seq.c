#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "dll.h"

#define SCORE_LIMIT 50
#define BLOCKSIZE 50

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t finishedlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ergebnisseBereit = PTHREAD_COND_INITIALIZER;
struct list_head *globalResults;
int *finished;

struct AlignJob{
	int work;
	int startCoordA;
	int lengthB;
	int id;
	char *sequence1,*sequence2;
	struct list_head *results;
};
struct Result{
	struct list_head head;
	int startA;
	int startB;
	int score;
};
struct GlobalResult{
	struct list_head head;
	struct list_head *content;
};

void print_results(struct list_head *list){
	struct list_head *start = list;
	struct Result *result = (struct Result *)list;
	struct list_head *temp = list->next;	
	while (temp != start){
		result = (struct Result *)temp;
		printf("%i %i %i\n\r",result->startA,result->startB,result->score);
		temp = temp->next;
	}
};
void print_max_Element(struct list_head *list){
	struct list_head *element = list->next;
	struct GlobalResult *globalResult = (struct GlobalResult *)element;

	// ResultListe beziehen
	struct list_head *resultListe = globalResult->content;
	struct list_head *temp = resultListe->next;	
	
	// Maximum finden
	int maxScore = 0;
	int startA,startB;
	struct Result *result;

	while (temp != resultListe){
		result = (struct Result *)temp;
		temp = temp->next;
		if (result->score >= maxScore){
			maxScore = result->score;
			startA = result->startA;
			startB = result->startB; 
		}
		free(result);
	}
	printf("%i %i %i\n",startA,startB,maxScore);

	// Berechnetes Element aus der globalenListe löschen
	list_del(element);


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

	int editDistanceDynamic(char *stringA, char *stringB){

		int lenA = strlen(stringA);
		int lenB = strlen(stringB);
		int editMatrix[lenA+1][lenB+1];
		int i,j;
		int max = 0;
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

			//Maximal Score für lokales Alignment suchen
				if (editMatrix[i][j]>= max){
					max = editMatrix[i][j]; 
				}
			}
		}

	// Ausgabe des Edit-Scores
		return max;
	}

	void *scoreCalculatorThread(void *arg){
		struct AlignJob *job = arg;
		char *stringA = job->sequence1;
		char *stringB = job->sequence2;
		int lengthB = job->lengthB;
		struct list_head *resultList = malloc(sizeof(struct list_head));
		list_init(resultList);
		job->results = resultList;
		int verschiebungenA = strlen(stringA)-BLOCKSIZE;
		char *subA,*subB;
		int i;
		int j;
		int verschiebungenB;
		struct Result *res;
		struct GlobalResult *gresult;

	// Äußere Schleife geht StringA blockweise durch
		for (i = 0; i < verschiebungenA; ++i){
			subA = malloc(BLOCKSIZE);
			memcpy(subA, stringA+i,BLOCKSIZE);
		// Durchgehen des zweiten Strings

			verschiebungenB = lengthB - BLOCKSIZE;
			for (j = 0; j < verschiebungenB; ++j){
				subB = malloc(BLOCKSIZE);
				memcpy(subB, stringB+j,BLOCKSIZE);

				int score = editDistanceDynamic(subA,subB);
				if (score > 50){
				// Result Element für die Speicherung der Ergebnisse mit Koordinaten
					res = malloc(sizeof(struct Result));
					res->score= score;
					res->startA= job->startCoordA + i;
					res->startB= j;
					list_add(&(res->head),resultList);
				}
				free(subB);
			}
			free(subA);
		// Ergebnisliste ist nicht leer also Resultatliste an Mainthread weiterleiten
			if (list_empty(resultList) == 0){
				gresult = malloc(sizeof(struct GlobalResult));
				gresult->content = resultList;

				pthread_mutex_lock(&lock);

				list_add((struct list_head *)gresult,globalResults);
				pthread_cond_signal(&ergebnisseBereit);

				pthread_mutex_unlock(&lock);
				resultList = malloc(sizeof(struct list_head));	
				list_init(resultList);

			}
		}
		free(job);
		pthread_mutex_lock(&finishedlock);
		finished[job->id]= 1;
		pthread_mutex_unlock(&finishedlock);

		pthread_mutex_lock(&lock);
		pthread_cond_signal(&ergebnisseBereit);
		pthread_mutex_unlock(&lock);
		// print_results(resultList);
		return NULL;
	}


	int main (int argc, char *argv[]){
		if (argv[1]==NULL || argv[2]==NULL){
			printf("Keine Eingabesequenzen, Benutzung: Datei1 Datei2 Threadanzahl\n");
			return -1;
		}
		
		int threadNumber;
		if (argv[3]){
			threadNumber  = atoi(argv[3]);
		} else {
			threadNumber  = 1;
			printf("Keine Threadanzahl angegeben. Standardmäßg auf 1 gesetzt.\n");
		}

	// Array für alle Thread-IDs
		pthread_t threads[threadNumber];
		globalResults = malloc(sizeof(struct list_head));
		list_init(globalResults);

	// Sequenzen Importieren
		char *seqA = importSequence(argv[1]);
		char *seqB = importSequence(argv[2]);
	// char *seqA = "CCGTCCGTTAATTCCTCTTGCATTCATATCGCGTATTTTTGTCTCTTTACCCGCTTACTTGGATAAGGATGACATAGCTTCTTACCGGAGCGCCTCCGTAAAATTGC";
	// char *seqB = "CTGGCAACCGGGAGGTGGGAATCCGTCACATATGAGAAGGTATTTGCCCGATAATCAATACTCCAGGCATCTAACTTTTCCCACTGCCTTAAGCCGGCTT";
		int lengthA = strlen(seqA);
		int lengthB = strlen(seqB);

		if (lengthA>=lengthB){
			int workPerThread = lengthA/threadNumber;
			if (workPerThread< BLOCKSIZE){
				printf("Zu viele Threads für Sequenzlänge eingestellt. Passende Anzahl wird ermittelt.\n");
				while(workPerThread < BLOCKSIZE){
					threadNumber -=1;
					workPerThread = lengthA/threadNumber;
				}
			}

			int rest = lengthA % threadNumber;
			finished = malloc(sizeof(int)*threadNumber);
			int i;
			char *cutoutA;
			char *cutoutB;
			struct AlignJob *job;
			int arbeitsbereich;

			printf("--- Berechnung mit %i Threads startet ---\n",threadNumber );
			for (i = 0; i < threadNumber; ++i){
				job = malloc(sizeof(struct AlignJob));
				arbeitsbereich = workPerThread + BLOCKSIZE;
			// Normale Aufteilung der Sequenz A für den Thread
				cutoutA = malloc(arbeitsbereich);
				memcpy( cutoutA, seqA+(i * workPerThread), arbeitsbereich);

				if (i == threadNumber-1){
					arbeitsbereich = workPerThread;
					if (rest > 0){
						cutoutA = malloc(arbeitsbereich + rest);
						memcpy( cutoutA, seqA+(i * workPerThread), arbeitsbereich + rest);
					}
				}
			// printf("%i %i %i %i\n",workPerThread, threadNumber,rest, arbeitsbereich );
			// Ausschnitt der Sequenz B
				cutoutB = malloc(lengthB+1);
				memcpy( cutoutB, seqB,lengthB);

			// Job erstellen
				job->sequence1 = cutoutA;
				job->sequence2 = cutoutB;
				job->lengthB = lengthB;
				job->work = workPerThread;
				job->startCoordA = i * workPerThread;
				job->id = i;
				pthread_mutex_lock(&finishedlock);
				finished[i]=0; 
				pthread_mutex_unlock(&finishedlock);
				pthread_create(&threads[i], NULL, scoreCalculatorThread, job);
			}

		// Verarbeitungsblock
			pthread_mutex_lock(&lock);
			int sum =0;
			while(list_empty(globalResults)){
				pthread_cond_wait(&ergebnisseBereit,&lock);

			// Checken ob alle Threads fertig sind
				sum = 0;
				pthread_mutex_lock(&finishedlock);
				for (i = 0; i < threadNumber; ++i){
					sum += finished[i];
				}
				pthread_mutex_unlock(&finishedlock);
			// Ergebnisliste abarbeiten
				while(list_empty(globalResults) == 0){
					print_max_Element(globalResults);
				}
			// Schleife verlassen wenn alles erledigt wurde
				if (sum == threadNumber){
					break;	
				}
			}
			pthread_mutex_unlock(&lock);

		// Threads joinen
			for (i = 0; i < threadNumber; ++i){
				pthread_join(threads[i], NULL);
			}

		} else {
			printf("Die erste Sequenz muss die Längere sein!\n" );
		}
		return 0;
	}
