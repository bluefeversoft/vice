// pvtable.c

#include "stdio.h"
#include "defs.h"

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
    tableEntry->posKey = 0ULL;
    tableEntry->move = NOMOVE;
    tableEntry->depth = 0;
    tableEntry->score = 0;
    tableEntry->flags = 0;
    tableEntry->age = 0;
  }
  table->newWrite=0;
  table->currentAge=0;
}

void InitHashTable(S_HASHTABLE *table, const int MB) {  
	
	int HashSize = 0x100000 * MB;
    table->numEntries = HashSize / sizeof(S_HASHENTRY);
    table->numEntries -= 2;
	
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
    ASSERT(alpha>=-INF_BOUND&&alpha<=INF_BOUND);
    ASSERT(beta>=-INF_BOUND&&beta<=INF_BOUND);
    ASSERT(pos->ply>=0&&pos->ply<MAXDEPTH);
	
	if( table->pTable[index].posKey == pos->posKey ) {
		*move = table->pTable[index].move;
		if(table->pTable[index].depth >= depth){
			table->hit++;
			
			ASSERT(table->pTable[index].depth>=1&&table->pTable[index].depth<MAXDEPTH);
            ASSERT(table->pTable[index].flags>=HFALPHA&&table->pTable[index].flags<=HFEXACT);
			
			*score = table->pTable[index].score;
			if(*score > ISMATE) *score -= pos->ply;
            else if(*score < -ISMATE) *score += pos->ply;
			
			switch(table->pTable[index].flags) {

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
    ASSERT(score>=-INF_BOUND&&score<=INF_BOUND);
    ASSERT(pos->ply>=0&&pos->ply<MAXDEPTH);
	
	int replace = FALSE;
	
	if( table->pTable[index].posKey == 0) {
		table->newWrite++;
		replace = TRUE;
	} else {
		if(table->pTable[index].age < table->currentAge ||
			table->pTable[index].depth < depth) {
				replace = TRUE;
			}
	}

	if (replace == FALSE) return;
	
	if(score > ISMATE) score += pos->ply;
    else if(score < -ISMATE) score -= pos->ply;
	
	table->pTable[index].move = move;
    table->pTable[index].posKey = pos->posKey;
	table->pTable[index].flags = flags;
	table->pTable[index].score = score;
	table->pTable[index].depth = depth;
}

int ProbePvMove(const S_BOARD *pos, const S_HASHTABLE *table) {

	int index = pos->posKey % table->numEntries;
	ASSERT(index >= 0 && index <= table->numEntries - 1);
	
	if( table->pTable[index].posKey == pos->posKey ) {
		return table->pTable[index].move;
	}
	
	return NOMOVE;
}
















