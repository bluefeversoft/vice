// pvtable.c

#include "stdio.h"
#include "defs.h"


#define EXTRACT_SCORE(x) ((x & 0xFFFF) - INF_BOUND)
#define EXTRACT_DEPTH(x) ((x >> 16) & 0x3F)
#define EXTRACT_FLAGS(x) ((x >> 23) & 0x3)
#define EXTRACT_MOVE(x) ((int)(x >> 25))

#define FOLD_DATA(sc, de, fl, mv) ( (sc + INF_BOUND) | (de << 16) | (fl << 23)  | ((U64)mv << 25))


void DataCheck(const int move) {
	int depth = rand() % MAXDEPTH;
	int flag = rand() % 3;
	int score = rand() % AB_BOUND;

	U64 data = FOLD_DATA(score, depth, flag, move);
	printf("Orig: move:%s d:%d fl:%d sc:%d data:%llX\n", PrMove(move), depth, flag, score, data);
	printf("Check: move:%s d:%d fl:%d sc:%d\n\n", 
	PrMove(EXTRACT_MOVE(data)), 
	EXTRACT_DEPTH(data), 
	EXTRACT_FLAGS(data), 
	EXTRACT_SCORE(data));
}


void TempHashTest(char *fen) {
	S_BOARD b[1];
	ParseFen(fen, b);

    S_MOVELIST list[1];
    GenerateAllMoves(b,list);
      
    int MoveNum = 0;
	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {	
       
        if ( !MakeMove(b,list->moves[MoveNum].move))  {
            continue;
        }
        TakeMove(b);
		DataCheck(list->moves[MoveNum].move);
    }
}


void VerifyEntrySMP(S_HASHENTRY *entry) {
    /*
	U64 data = FOLD_DATA(entry->score, entry->depth, entry->flags, entry->move);
	U64 key = entry->posKey ^ data;

	if (data != entry->smp_data) { printf("data error"); exit(1);}
	if (key != entry->smp_key) { printf("smp_key error"); exit(1);}

	int move = EXTRACT_MOVE(data);
	int flag = EXTRACT_FLAGS(data);
	int score = EXTRACT_SCORE(data);
	int depth = EXTRACT_DEPTH(data);
	
	if (move != entry->move) { printf("move error"); exit(1);}
	if (flag != entry->flags) { printf("flags error"); exit(1);}
	if (score != entry->score) { printf("score error"); exit(1);}
	if (depth != entry->depth) { printf("depth error"); exit(1);}
    */
}




S_HASHTABLE HashTable[1];


int GetPvLine(const int depth, S_BOARD *pos, const S_HASHTABLE *table) {

	ASSERT(depth < MAXDEPTH && depth >= 1);

	int move = ProbePvMove(pos, table);
	int count = 0;
	
	while(move != NOMOVE && count < depth) {
	
		ASSERT(count < MAXDEPTH);
	
		if( MoveExists(pos, move) ) {
			MakeMove(pos, move);
			pos->PvArray[count++] = move;
		} else {
			break;
		}		
		move = ProbePvMove(pos, table);	
	}
	
	while(pos->ply > 0) {
		TakeMove(pos);
	}
	
	return count;
	
}

void ClearHashTable(S_HASHTABLE *table) {

  S_HASHENTRY *tableEntry;
  
  for (tableEntry = table->pTable; tableEntry < table->pTable + table->numEntries; tableEntry++) {
   /* tableEntry->posKey = 0ULL;
    tableEntry->move = NOMOVE;
    tableEntry->depth = 0;
    tableEntry->score = 0;
    tableEntry->flags = 0;*/
    tableEntry->age = 0;
	tableEntry->smp_data = 0ULL;
	tableEntry->smp_key = 0ULL;
  }
  table->newWrite=0;
  table->currentAge=0;
}

void InitHashTable(S_HASHTABLE *table, const int MB) {  
	
	int HashSize = 0x100000 * MB;
    table->numEntries = HashSize / sizeof(S_HASHENTRY);
    table->numEntries -= 2;
    //table->numEntries = 1000000;
	
	if(table->pTable!=NULL) {
		free(table->pTable);
	}
		
    table->pTable = (S_HASHENTRY *) malloc(table->numEntries * sizeof(S_HASHENTRY));
	if(table->pTable == NULL) {
		printf("Hash Allocation Failed, trying %dMB...\n",MB/2);
		InitHashTable(table,MB/2);
	} else {
		ClearHashTable(table);
		printf("HashTable init complete with %d entries\n",table->numEntries);
	}
	
}

int ProbeHashEntry(S_BOARD *pos, S_HASHTABLE *table, int *move, int *score, int alpha, int beta, int depth) {

	int index = pos->posKey % table->numEntries;
	
	ASSERT(index >= 0 && index <= table->numEntries - 1);
    ASSERT(depth>=1&&depth<MAXDEPTH);
    ASSERT(alpha<beta);
    ASSERT(alpha>=-AB_BOUND&&alpha<=AB_BOUND);
    ASSERT(beta>=-AB_BOUND&&beta<=AB_BOUND);
    ASSERT(pos->ply>=0&&pos->ply<MAXDEPTH);

	U64 test_key = pos->posKey ^ table->pTable[index].smp_data;
	if( table->pTable[index].smp_key == test_key ) {

		int smp_depth = EXTRACT_DEPTH(table->pTable[index].smp_data);
		int smp_move = EXTRACT_MOVE(table->pTable[index].smp_data);
		int smp_score = EXTRACT_SCORE(table->pTable[index].smp_data);
		int smp_flags = EXTRACT_FLAGS(table->pTable[index].smp_data);


		*move = smp_move;
		if(smp_depth >= depth){
			table->hit++;
			
			//ASSERT(table->pTable[index].depth>=1&&table->pTable[index].depth<MAXDEPTH);
            //ASSERT(table->pTable[index].flags>=HFALPHA&&table->pTable[index].flags<=HFEXACT);
			
			*score = smp_score;
			if(*score > ISMATE) *score -= pos->ply;
            else if(*score < -ISMATE) *score += pos->ply;
			
			switch(smp_flags) {

                case HFALPHA: if(*score<=alpha) {
                    *score=alpha;
                    return TRUE;
                    }
                    break;
                case HFBETA: if(*score>=beta) {
                    *score=beta;
                    return TRUE;
                    }
                    break;
                case HFEXACT:
                    return TRUE;
                    break;
                default: ASSERT(FALSE); break;
            }
		}
	}
	
	return FALSE;
}

void StoreHashEntry(S_BOARD *pos, S_HASHTABLE *table, const int move, int score, const int flags, const int depth) {

	int index = pos->posKey % table->numEntries;	

	/*if (table->pTable[index].depth > 8 && depth < 8) { 
		printf("new_key:%llX old_key:%llX index:%d depth:%d replace:%d\n", 
		pos->posKey, table->pTable[index].posKey, index, depth, table->pTable[index].depth); 
	}*/
	
	ASSERT(index >= 0 && index <= table->numEntries - 1);
	ASSERT(depth>=1&&depth<MAXDEPTH);
    ASSERT(flags>=HFALPHA&&flags<=HFEXACT);
    ASSERT(score>=-AB_BOUND&&score<=AB_BOUND);
    ASSERT(pos->ply>=0&&pos->ply<MAXDEPTH);
	
	int replace = FALSE;
	
	if( table->pTable[index].smp_key == 0) {
		table->newWrite++;
		replace = TRUE;
	} else {
		if(table->pTable[index].age < table->currentAge) {
            replace = TRUE;
        } else if(EXTRACT_DEPTH(table->pTable[index].smp_data) <= depth) {
            replace = TRUE;
        }
	}

	if (replace == FALSE) return;
	
	if(score > ISMATE) score += pos->ply;
    else if(score < -ISMATE) score -= pos->ply;

	U64 smp_data = FOLD_DATA(score, depth, flags, move);
	//U64 smp_key = pos->posKey ^ smp_data;
	
	/*table->pTable[index].move = move;
    table->pTable[index].posKey = pos->posKey;
	table->pTable[index].flags = flags;
	table->pTable[index].score = score;
	table->pTable[index].depth = depth;*/
	table->pTable[index].age = table->currentAge;
	table->pTable[index].smp_data = smp_data;
	table->pTable[index].smp_key = pos->posKey ^ smp_data;

}

int ProbePvMove(const S_BOARD *pos, const S_HASHTABLE *table) {

	int index = pos->posKey % table->numEntries;

    U64 test_key = pos->posKey ^ table->pTable[index].smp_data;

	ASSERT(index >= 0 && index <= table->numEntries - 1);
	
	if( table->pTable[index].smp_key == test_key ) {
		return EXTRACT_MOVE(table->pTable[index].smp_data);
	}
	
	return NOMOVE;
}
















