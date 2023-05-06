/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include <new>
#include "bp_api.h"

class Row{
public:
    uint32_t tag;
    uint32_t target;
    unsigned history;
    unsigned* localTable;
    Row():tag(-1), history(0),localTable(nullptr){};
    ~Row(){
        if(localTable != nullptr) delete localTable;
    }
};
/********
 * our main class of the predictor which contains all the important features we want to hold like the BTB, global FSM table and global history
 */
class Predictor
{
public:
    unsigned btbSize;
    unsigned historySize;
    unsigned tagSize;
    unsigned fsmInintialState;
    bool isGlobalHist;
    bool isGlobalTable;
    int Shared;
    unsigned* globalTable;
    Row* btb;
    unsigned globalHistory;
    unsigned flushes;
    unsigned update_calls;
    bool taken;
    Predictor(unsigned btbSizee, unsigned historySizee, unsigned tagSizee, unsigned fsmStatee,
              bool isGlobalHistt, bool isGlobalTablee, int Sharedd):
            btbSize(btbSizee), historySize(historySizee), tagSize(tagSizee), fsmInintialState(fsmStatee),
            isGlobalHist(isGlobalHistt), isGlobalTable(isGlobalTablee), Shared(Sharedd), globalHistory(0),flushes(0),update_calls(0),taken(false){}
    ~Predictor(){
        delete[] btb;
        if (globalTable != nullptr) delete globalTable;
    }


};

Predictor* predictor;



static unsigned powerOfTwo(unsigned size)
{
    unsigned result = 1;
    for (unsigned i = 0; i < size; i++)
    {
        result *= 2;
    }
    return result;
}

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
            bool isGlobalHist, bool isGlobalTable, int Shared){
    try{
        predictor = new Predictor(btbSize,historySize,tagSize,fsmState,isGlobalHist,isGlobalTable, Shared);
        predictor->btb = new Row[btbSize];
        unsigned tableSize = powerOfTwo(historySize);

        if(isGlobalHist) predictor->globalHistory = 0;
        if(isGlobalTable){
            predictor->globalTable = new unsigned[tableSize];
            for (unsigned i = 0; i < tableSize; ++i)
            {
                predictor->globalTable[i] = fsmState;
            }
        }else{
            for (unsigned i = 0; i < btbSize; i++)
            {
                predictor->btb[i].localTable = new unsigned[tableSize];
                for (unsigned j = 0; j < tableSize; ++j)
                {
                    predictor->btb[i].localTable[j] = fsmState;
                }
            }
        }
    }catch(std::bad_alloc& e){
        return -1;
    }
    return 0;
}

unsigned getStartOfTag(int btbSize)
{
    unsigned startOfTag=0;
    switch (btbSize) {
        case 1:
            break;
        case 2:
            startOfTag = 1;
            break;
        case 4:
            startOfTag = 2;
            break;
        case 8:
            startOfTag = 3;
            break;
        case 16:
            startOfTag = 4;
            break;
        case 32:
            startOfTag = 5;
            break;
    }
    return startOfTag +2;
}



uint32_t getTag(uint32_t pc, unsigned startOfTag, unsigned tagSize)
{
    uint32_t tmp =0;
    pc = pc >> startOfTag;
    for (unsigned i = 0; i < tagSize; ++i)
    {
        tmp += 1;
        tmp = tmp <<1;
    }
    tmp = tmp >>1;
    tmp = pc & tmp;
    return  tmp;

}


uint32_t getEntry(uint32_t pc, unsigned btbEntrySize)
{
    uint32_t tmp =0;
    pc = pc >> 2;
    for (unsigned i = 0; i < btbEntrySize; ++i)
    {
        tmp += 1;
        tmp = tmp <<1;
    }
    tmp = tmp >> 1;
    tmp = pc & tmp;
    return  tmp;
}

bool predict(unsigned fsmState, uint32_t *dst,uint32_t pc,uint32_t target)
{
    switch (fsmState) {
        case 0:
            *dst = pc+ 4;
            predictor->taken = false;
            return false;
        case 1:
            *dst = pc+ 4;
            predictor->taken = false;
            return false;
        case 2:
            *dst = target;
            predictor->taken = true;
            return true;
        case 3:
            *dst = target;
            predictor->taken = true;
            return true;
    }
    *dst = pc+ 4;
    predictor->taken = false;
    return false;
}
unsigned shareLSB(uint32_t pc, unsigned history){
    pc = pc >> 2;
    uint32_t tmp = 0;
    for (unsigned i = 0; i < predictor->historySize; ++i)
    {
        tmp += 1;
        tmp = tmp <<1;
    }
    tmp = tmp>>1;
    tmp = pc & tmp;
    return  (history ^ tmp);
}

unsigned shareMid(unsigned history,uint32_t pc){
    pc = pc >> 16;
    uint32_t tmp = 0;
    for (unsigned i = 0; i < predictor->historySize; ++i)
    {
        tmp += 1;
        tmp = tmp <<1;
    }
    tmp = tmp >>1;
    tmp = pc & tmp;
    return  history ^ tmp;
}
/*******************
 * @param pc
 * @param btbEntry
 * @return
 * returns the index in the FSM table according to it's share option (lsb or mid ot no share) used in global table
 */
unsigned getIndex(uint32_t pc,uint32_t btbEntry) {
    unsigned indexInTable;
    if (!(predictor->isGlobalTable)) {
        if (predictor->isGlobalHist) {
            indexInTable = predictor->globalHistory;
        } else {
            indexInTable = predictor->btb[btbEntry].history;
        }
    } else {
        if (predictor->Shared == 1) {
            if (predictor->isGlobalHist) {
                indexInTable = shareLSB(pc, predictor->globalHistory);
            } else {
                indexInTable = shareLSB(pc, predictor->btb[btbEntry].history);
            }
        }
        if (predictor->Shared == 2) {
            if (predictor->isGlobalHist) {
                indexInTable = shareMid(predictor->globalHistory, pc);
            } else {
                indexInTable = shareMid(predictor->btb[btbEntry].history, pc);
            }
        }
        if (predictor->Shared == 0) {
            if (predictor->isGlobalHist) {
                indexInTable = predictor->globalHistory;
            } else {
                indexInTable = predictor->btb[btbEntry].history;
            }
        }
    }
    return indexInTable;
}
/************************BP PREDICT**********************
 * @param pc
 * @param dst
 * @return
 * The function predicts the state of the branch according to it's FSM table (if it's local) or the global FSM table
 * if it's public, it predicts the FSM state according to it's history as we learned in the lectures :)
 */

bool BP_predict(uint32_t pc, uint32_t *dst){
    unsigned startOfTag = getStartOfTag(predictor->btbSize);
    uint32_t tagToSearch = getTag(pc,startOfTag,predictor->tagSize);
    unsigned btbEntrySize = getStartOfTag(predictor->btbSize) -2;
    uint32_t btbEntry = getEntry(pc,btbEntrySize);
    if(predictor->btb[btbEntry].tag != tagToSearch)
    {
        *dst = pc +4;
        predictor->taken = false;
        return false;
    }
    uint32_t target = predictor->btb[btbEntry].target;
    unsigned indexInTable =getIndex(pc,btbEntry);
    if(!(predictor->isGlobalTable))
    {
        unsigned fsmState= predictor->btb[btbEntry].localTable[indexInTable];
        return predict(fsmState,dst,pc,target);
    }else{
        return predict(predictor->globalTable[indexInTable],dst,pc,target);
    }

}
/******************
 * @param FSMState
 * @param taken
 * a function to update the FSM state according to the FSM table we leaned in the lecture which have 4 states ST WT WNT SNT
 */
void updateFSM(unsigned* FSMState,bool taken){
    if(taken){
        if((*FSMState) < 3){
            (*FSMState)++;
        }
    }
    else{
        if ((*FSMState)>0){
            (*FSMState)--;
        }
    }
}
/******************
 * @param history
 * @param taken
 * the function updates the history of the branch by shifting left and adding 1 or 0 according to it's state taken or not.
 */
void updateHist(unsigned* history,bool taken){
    unsigned tmp = powerOfTwo(predictor->historySize ) -1;
    *history = (*history) << 1;
    if(taken){
        (*history)++;
    }
    *history = (*history) & tmp;
}
/*************************************BP UPDATE****************************************
 * @param pc
 * @param targetPc
 * @param taken
 * @param pred_dst
 * The function updates the BTB by adding a new pc or updating an existing one by changing it's history or the FSM table
 * if it's local or global
 */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
    if(taken != predictor->taken || (predictor->taken && pred_dst != targetPc)){
        predictor->flushes++;
    }
    predictor->update_calls++;
    unsigned startOfTag = getStartOfTag(predictor->btbSize);
    uint32_t tagToSearch = getTag(pc,startOfTag,predictor->tagSize);
    unsigned btbEntrySize = getStartOfTag(predictor->btbSize) -2;
    uint32_t btbEntry = getEntry(pc,btbEntrySize);
    unsigned indexInTable = getIndex(pc,btbEntry);
    if(predictor->btb[btbEntry].tag != tagToSearch) {
        predictor->btb[btbEntry].tag = tagToSearch;
        predictor->btb[btbEntry].target = targetPc;
        unsigned tableSize =powerOfTwo(predictor->historySize);
        if (!predictor->isGlobalHist) {
            predictor->btb[btbEntry].history = 0;
            indexInTable = getIndex(pc,btbEntry);
        }

        if (!predictor->isGlobalTable) {
            for (unsigned i = 0; i <tableSize ; ++i) {
                predictor->btb[btbEntry].localTable[i] = predictor->fsmInintialState;
            }
            updateFSM(&(predictor->btb[btbEntry].localTable[indexInTable]),taken);
        }else{
            updateFSM(&(predictor->globalTable[indexInTable]),taken);
        }

        if (!predictor->isGlobalHist) {
            updateHist(&predictor->btb[btbEntry].history,taken);
        }else{
            updateHist(&predictor->globalHistory,taken);
        }
    }
    else {
        if(targetPc != pred_dst) predictor->btb[btbEntry].target = targetPc;

        if(predictor->isGlobalTable){
            updateFSM(&predictor->globalTable[indexInTable],taken);
            if(predictor->isGlobalHist){
                updateHist(&predictor->globalHistory,taken);
            }
            else{
                updateHist(&predictor->btb[btbEntry].history,taken);
            }
        }
        else{
            updateFSM(&predictor->btb[btbEntry].localTable[indexInTable],taken);
            if(predictor->isGlobalHist){
                updateHist(&predictor->globalHistory,taken);
            }
            else{
               updateHist(&predictor->btb[btbEntry].history,taken);
            }
        }
    }
}


void BP_GetStats(SIM_stats *curStats){
    curStats->br_num = predictor->update_calls;
    curStats->flush_num = predictor->flushes;
    if(predictor->isGlobalHist && predictor->isGlobalTable)
    {
        curStats->size = predictor->btbSize * (1 + predictor->tagSize+ 30 ) +
                         predictor->historySize+ 2*powerOfTwo(predictor->historySize);
    }

    if(predictor->isGlobalHist && !predictor->isGlobalTable)
    {
        curStats->size = predictor->btbSize * (1 + 30+ predictor->tagSize+ 2* powerOfTwo(predictor->historySize)) +
                         predictor->historySize;
    }

    if(!predictor->isGlobalHist && predictor->isGlobalTable)
    {
        curStats->size = predictor->btbSize * (1 +30+ predictor->tagSize+predictor->historySize )
                         + 2*powerOfTwo(predictor->historySize);
    }
    if(!predictor->isGlobalHist && !predictor->isGlobalTable)
    {
        curStats->size = predictor->btbSize * (1+ 30+predictor->tagSize+predictor->historySize+ 2* powerOfTwo(predictor->historySize) );
    }

}
