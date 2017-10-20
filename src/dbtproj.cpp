#include "dbtproj.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

struct linkedList {
    record_t current;
    linkedList *next;
};

void printFile(char *filename, int nblocks){
        FILE *in;
        block_t block;
        int times=0;
        in = fopen(filename, "r");
        while (!feof(in)) {
            fread(&block, 1, sizeof(block_t), in);// read the next block
            for (int i=0; i<block.nreserved; ++i) {  // print block contents
                    if (block.entries[i].valid)
                        cout<<"block id: "<<block.blockid<<", record id: "<<block.entries[i].recid<<", num: "<<block.entries[i].num<<", str: "<<block.entries[i].str<<endl;
            }
            times+=1;
            if(times==nblocks) break;
        }
        fclose(in);
}

//fill the sortvalues array with values of a certain block
void fillarray2(block_t *buffer, uint *sortvalues, char **sortvalues2, unsigned char field, uint block)
{
    int l = block * buffer[0].nreserved;

    if(field == 0){
            for (uint j=0; j < buffer[block].nreserved; ++j) {
                sortvalues[l] =  buffer[block].entries[j].recid;
                l++;
            }
    }
    else if (field == 1){
            for (uint j=0; j < buffer[block].nreserved; ++j) {
                sortvalues[l] =  buffer[block].entries[j].num;
                l++;
            }
    }
    else if (field == 2){
            for (uint j=0; j < buffer[block].nreserved; ++j) {
                strcpy(sortvalues2[l], buffer[block].entries[j].str);
                l++;
            }
    }
    else{
            for (uint j=0; j < buffer[block].nreserved; ++j) {
                sortvalues[l]=  buffer[block].entries[j].num;
                strcpy(sortvalues2[l], buffer[block].entries[j].str);
                l++;
            }
    }
}

void merging(int &input, int &output, block_t *buffer, uint sizeMemory, uint numMergingSegs, uint *blocksLeft, uint segmentSize, uint firstId, unsigned char field, bool lastMerge, uint *sortvalues, char **sortvalues2, uint *nios) {

    uint leftSegsize; //number of blocks in segment has
    uint outblocks = 0;
    uint numMerge = numMergingSegs;
    int recid=0; // record id
    int nextminRec; // position of next minimum record in sortedvalues array
    int minRec; // integer value of minimum record
    char *minRec2=new char[STR_LENGTH]; // string value of minimum record
    if (lastMerge == true)
        leftSegsize = blocksLeft[numMergingSegs - 1] + 1;
    block_t *last = buffer + sizeMemory; // pointer to the last block of buffer
    for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++) // empties the last block
        last->entries[i].valid = false;
    last->nreserved = 0;
    last->blockid = 0;

     while (numMerge != 0) {
        uint i;
        // finds the first valid block
        for (i = 0; i < numMergingSegs; i++) {
            if (buffer[i].valid == true)
                break;
        }
        // get the position of minimum record in sortedvalues array and the record id in buffer
        for(uint k = 0; k < buffer[i].nreserved; k++){
            if(buffer[i].entries[k].valid == true){
                nextminRec = i*buffer[0].nreserved+k;
                recid = nextminRec%buffer[i].nreserved;
                break;
            }
        }
        // save the value of minimum record
        if(field == 0 || field == 1){
            minRec = sortvalues[nextminRec];
        }
        else if (field == 2){
            strcpy(minRec2,sortvalues2[nextminRec]);
        }
        else{
            minRec = sortvalues[nextminRec];
            strcpy(minRec2,sortvalues2[nextminRec]);
        }
        // compare each value of the current minimum record with the next records of each segment and find the minimum record
        uint minId = i;
        for (uint j = i + 1; j < numMergingSegs; j++) {
            //find the next valid minimum record
            for(uint k = 0; k < buffer[j].nreserved; k++){
                if(buffer[j].entries[k].valid == true){
                    nextminRec = j*buffer[0].nreserved+k;
                    break;
                }
            }
           //compare records and find minimum
            if(field == 0 || field == 1){
                if (buffer[j].valid && sortvalues[nextminRec]<minRec) {
                    minRec = sortvalues[nextminRec];
                    recid=nextminRec%buffer[j].nreserved;
                    minId = j;
                }
            }
            else if(field == 2){
                if (buffer[j].valid && strcmp(sortvalues2[nextminRec],minRec2)<0) {
                    strcpy(minRec2,sortvalues2[nextminRec]);
                    recid=nextminRec%buffer[j].nreserved;
                    minId = j;
                }
            }
            else{
                if(buffer[j].valid && ((sortvalues[nextminRec]<minRec) || (sortvalues[nextminRec]==minRec && strcmp(sortvalues2[nextminRec],minRec2)<0))){
                    minRec = sortvalues[nextminRec];
                    strcpy(minRec2,sortvalues2[nextminRec]);
                    recid=nextminRec%buffer[j].nreserved;
                    minId = j;
                }
            }
        }

        // minimum record is written to the last block, which is used as output
        last->entries[last->nreserved++] = buffer[minId].entries[recid];
        buffer[minId].entries[recid].valid = false;

        // if the last block is full, write it to the outfile
        if (last->nreserved == MAX_RECORDS_PER_BLOCK) {
            write(output, last, sizeof (block_t));
            (*nios) += 1;
            last->blockid += 1;
            outblocks += 1;
            for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++) { //empties last block
                last->entries[i].valid = false;
            }
            last->nreserved = 0;
        }


        uint  blockId;
        if ((recid+1)-buffer[minId].nreserved==0) { // if we compared all the values of this block
            if (blocksLeft[minId] > 0) {
                if ((lastMerge == true) && minId == numMergingSegs - 1)
                    blockId = firstId + segmentSize * minId + leftSegsize - blocksLeft[minId];
                else
                    blockId = firstId + segmentSize * minId + segmentSize - blocksLeft[minId];
                pread(input, buffer + minId, sizeof (block_t), blockId * sizeof (block_t)); //read the next block
                (*nios) +=1;
                fillarray2(buffer,sortvalues,sortvalues2,field,minId); //fill the sortvalues array with values of the new block
                for(uint k = 0; k < buffer[minId].nreserved; k++)
                    buffer[minId].entries[k].valid =true;

                blocksLeft[minId] -= 1;
                if (buffer[minId].valid == false)
                    numMerge -= 1;
            } else {
                buffer[minId].valid = false;
                numMerge -= 1;
            }
        }
        else {
            if (buffer[minId].entries[recid+1].valid == false) {
                buffer[minId].valid = false;
                numMerge -= 1;
            }

        }
     }


    // if merging is over and there are some records on the last block writes them on the output
    if (last->nreserved != 0) {
        write(output, last,last->nreserved * (sizeof (block_t)/(double)MAX_RECORDS_PER_BLOCK));
        (*nios) += 1;
        last->blockid += 1;
        outblocks += 1;
    }

    delete[] minRec2;
}

//sort the buffer
void sortbuffer(block_t *buffer, int i, int j){
   int records = buffer[0].nreserved;
   int k,l;
   record_t temp2;

   if (i < records){
        k = 0;
   }
   else{
      k = i / records;
      i = i % records;
   }
   if(j < records){
         l = 0;
     }
     else{
        l = j / records;
        j = j % records;
   }
   temp2 = buffer[k].entries[i];
   buffer[k].entries[i] = buffer[l].entries[j];
   buffer[l].entries[j] = temp2;

}

// partition of quicksort
uint partition(block_t *buffer, uint *sortvalues, char **sortvalues2, int p, int q, unsigned char field)
{
    uint temp1;
    char *temp2=new char[STR_LENGTH];
    char *y=new char[STR_LENGTH];
    uint x= sortvalues[p];
    strcpy(y,sortvalues2[p]);
    uint i=p;
    uint j;

    for(j = p+1; j < q; j++)
    {
        if(field == 0 || field == 1){
            if(sortvalues[j] <= x)
            {
                i=i+1;
                temp1=sortvalues[i];
                sortvalues[i]=sortvalues[j];
                sortvalues[j]=temp1;
                sortbuffer(buffer,i,j);
            }

        }
        else if (field == 2){
            if(strcmp(y,sortvalues2[j])>0)
            {
                i=i+1;
                strcpy(temp2,sortvalues2[i]);
                strcpy(sortvalues2[i],sortvalues2[j]);
                strcpy(sortvalues2[j],temp2);
                sortbuffer(buffer,i,j);
            }
        }
        else{
            if((sortvalues[j] < x || (sortvalues[j] == x && strcmp(y,sortvalues2[j]) > 0))){
                i=i+1;
                temp1=sortvalues[i];
                sortvalues[i]=sortvalues[j];
                sortvalues[j]=temp1;
                sortbuffer(buffer,i,j);

            }
        }

    }
    if(field == 0 || field == 1 || field == 3){
        temp1=sortvalues[i];
        sortvalues[i]=sortvalues[p];
        sortvalues[p]=temp1;
        sortbuffer(buffer,i,p);
    }else{
        strcpy(temp2,sortvalues2[i]);
        strcpy(sortvalues2[i],sortvalues2[p]);
        strcpy(sortvalues2[p],temp2);
        sortbuffer(buffer,i,p);
    }
    delete[] temp2;delete[] y;
    return i;
}

// sort the sortvalues array using quicksort
void quickSort(block_t *buffer, uint *sortvalues, char **sortvalues2, int p, int q, unsigned char field)
{
    int r;
    if(p < q)
    {
        r=partition(buffer,sortvalues,sortvalues2, p,q,field);
        quickSort(buffer,sortvalues,sortvalues2,p,r,field);
        quickSort(buffer,sortvalues,sortvalues2,r+1,q,field);
    }
}

// create a array of integers
uint* createarray(block_t *buffer, int counter){
    unsigned int *sortarray = new unsigned int[counter];
    return sortarray;

}

// create a array of strings
char ** createarray2(block_t *buffer,int counter){
    char ** sortarray2 = new char*[counter];
    for(int i = 0; i < counter; ++i)
          sortarray2[i] = new char[STR_LENGTH];
    return sortarray2;
}

//fill the sortvalues array with values of each buffer record
void fillarray(block_t *buffer, int nmem_blocks, uint *sortvalues, char **sortvalues2, unsigned char field)
{
    int l=0;
    if(field ==0){
        for(uint i = 0; i < nmem_blocks; i++){
            for (int j=0; j<buffer[i].nreserved; ++j) {
                sortvalues[l]=  buffer[i].entries[j].recid;
                l++;
            }
            buffer[i].blockid = i;
        }
    }
    else if (field ==1){
        for(uint i = 0; i < nmem_blocks; i++){
            for (int j=0; j<buffer[i].nreserved; ++j) {
                sortvalues[l]=  buffer[i].entries[j].num;
                l++;
            }
            buffer[i].blockid = i;
        }
    }
    else if (field ==2){
        for(uint i = 0; i < nmem_blocks; i++){
            for (int j=0; j<buffer[i].nreserved; ++j) {
                strcpy(sortvalues2[l], buffer[i].entries[j].str);
                l++;
            }
            buffer[i].blockid = i;
        }
    }
    else{
        for(uint i = 0; i < nmem_blocks; i++){
            for (int j=0; j<buffer[i].nreserved; ++j) {
                sortvalues[l]=  buffer[i].entries[j].num;
                strcpy(sortvalues2[l], buffer[i].entries[j].str);
                l++;
            }
            buffer[i].blockid = i;
        }
    }
}


void dbtproj::MergeSort(char *infile, unsigned char field, block_t *buffer, unsigned int nmem_blocks, char *outfile, unsigned int *nsorted_segs, unsigned int *npasses, unsigned int *nios)
{
    *nios = 0;
    struct stat st;
    stat(infile, &st);
    uint size= st.st_size / sizeof (block_t); // file blocks
    for (uint i = 0; i < nmem_blocks; i++) { // empties the buffer
            for (int j = 0; j < MAX_RECORDS_PER_BLOCK; j++)
                    buffer[i].entries[j].valid = false;
        buffer[i].nreserved = 0;
        buffer[i].valid = true;
    }
    char tmp1[] = ".t1";
    char tmp2[] = ".t2";
    int input = open(infile, O_RDONLY, S_IRWXU);
    int output = open(tmp1, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

    uint fullbuffers = size / nmem_blocks; // number of segments that fill the buffer
    uint leftbuffers = size % nmem_blocks; // left blocks that doen't fill the buffer
    uint sortedbuffers = 0;
    uint bufferblocks = nmem_blocks;
    // sort each segment and write it to the temp file 1
    for (uint i = 0; i <= fullbuffers; i++) {
        if (fullbuffers == i) {
                if (leftbuffers != 0)
                    bufferblocks = leftbuffers;
                else
                    break;
        }
        read(input, buffer, bufferblocks * sizeof (block_t));
        (*nios) += bufferblocks;
        int counter=0;
        for(uint j = 0; j < bufferblocks; j++)
                counter= counter + buffer[j].nreserved; // find the valid number of records in the buffer
         uint *sortvalues = createarray(buffer,counter); //create an array of integers with the size of buffer (for field 0,1,3)
         char **sortvalues2 = createarray2(buffer,counter); //create an array of strings(for field 2,3)
         fillarray(buffer,bufferblocks,sortvalues,sortvalues2,field); //fill the array with the values of each record
         quickSort(buffer,sortvalues,sortvalues2,0,counter,field); //sort the array and at the same time sort the buffer
         delete sortvalues;
         for(int k = 0; k < counter; ++k)
             delete [] sortvalues2[k];
         delete [] sortvalues2;
         write(output, buffer, bufferblocks * sizeof (block_t));
         (*nios) += bufferblocks;
         (*nsorted_segs) += 1;
         sortedbuffers += 1;
    }
    (*npasses) += 1;
    close(input);
    close(output);


    uint numleftbuffers; //blocks of last segment
    if (leftbuffers == 0)
            numleftbuffers = nmem_blocks;
    else
            numleftbuffers = leftbuffers;
    bufferblocks = nmem_blocks;
    buffer[nmem_blocks - 1].valid = true;
    while (sortedbuffers > 1) {
        input = open(tmp1, O_RDONLY, S_IRWXU);
        output = open(tmp2, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        uint *blocksLeft = new unsigned int[(nmem_blocks - 1) * sizeof (uint)]; //number of blocks left of a segment
        uint tmpSortedbuffers = 0;
        uint numMergingSegs = nmem_blocks - 1;
        uint mergingSegs = sortedbuffers / (nmem_blocks - 1); // number of merges that fill the buffer
        uint leftMergingSegs = sortedbuffers % (nmem_blocks - 1); // number of merges that segment doesn't fill the buffer
        bool lastMerge = false; //check if it's the last merge
        for (uint j = 0; j <= mergingSegs; j++) {
            if ((j == mergingSegs - 1) && (leftMergingSegs == 0))
                    lastMerge = true;
            else if (j == mergingSegs) {
                    if (leftMergingSegs != 0) {
                        numMergingSegs = leftMergingSegs;
                        lastMerge = true;
                    } else
                        break;
            }
            int counter=0;
            // take the first block of each segment to merge on the buffer
            for (uint i = 0; i < numMergingSegs; i++) {
                    pread(input, buffer + i, sizeof (block_t),((j * (nmem_blocks - 1) * bufferblocks) + i * bufferblocks) * sizeof (block_t));
                    (*nios) +=1;
                    counter= counter + buffer[i].nreserved;
                    blocksLeft[i] = bufferblocks - 1;
            }
           uint *sortvalues = createarray(buffer,counter);
           char **sortvalues2 = createarray2(buffer,counter);
           fillarray(buffer,numMergingSegs,sortvalues,sortvalues2,field);

            if (lastMerge == true) // we get the number of  blocks of the last segment
                    blocksLeft[numMergingSegs - 1] = numleftbuffers - 1;

            merging(input, output, buffer, nmem_blocks - 1, numMergingSegs, blocksLeft,bufferblocks, j * (nmem_blocks - 1) * bufferblocks, field, lastMerge, sortvalues, sortvalues2, nios);

            tmpSortedbuffers += 1;
            delete sortvalues;
            for(int k = 0; k < counter; ++k)
                delete [] sortvalues2[k];
            delete [] sortvalues2;
        }
        delete[] blocksLeft;

        if (leftMergingSegs == 0)
            numleftbuffers = (nmem_blocks - 2)*bufferblocks + numleftbuffers;
        else
            numleftbuffers = (leftMergingSegs - 1) * bufferblocks + numleftbuffers;
        bufferblocks = bufferblocks * (nmem_blocks - 1);
        sortedbuffers = tmpSortedbuffers;
        (*npasses) += 1;
        // swaps the files for the next pass
        char tmp = tmp1[2];
        tmp1[2] = tmp2[2];
        tmp2[2] = tmp;
        close(input);
        close(output);
    }
    rename(tmp1, outfile); // rename outfile
    remove(tmp2);
    remove(tmp1);
}

void eliminateDuplicatesMerging(int &input, int &output, block_t *buffer, uint sizeMemory, uint numMergingSegs, uint *blocksLeft, uint segmentSize, uint firstId, unsigned char field, bool lastPass, bool lastMerge, unsigned int *sortvalues, char **sortvalues2, uint *nunique, uint *nios) {

    uint leftSegsize; //number of blocks in segment has
    uint numMerge = numMergingSegs;
    uint outblocks = 0;
    uint recid=0; // record id
    uint nextminRec; // position of next minimum record in sortedvalues array
    uint minRec; // integer value of minimum record
    char *minRec2=new char[STR_LENGTH];  // string value of minimum record
    uint dup =0;
    uint ndups;
    if (lastMerge == true)
        leftSegsize = blocksLeft[numMergingSegs - 1] + 1;
    block_t *last = buffer + sizeMemory; // pointer to the last block of buffer
    for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++) // empties the last block
        last->entries[i].valid = false;
    last->nreserved = 0;
    last->blockid = 0;

    while (numMerge != 0) {
        uint i,block=0;

        for (uint j = 0; j < numMergingSegs; j++) {
             if(buffer[j].valid == true)
                 block++;
        }
        // finds the first valid block
        for (i = 0; i < numMergingSegs; i++) {
            if (buffer[i].valid == true)
                break;
        }
        // get the position of minimum record in sortedvalues array and the record id in buffer
        for(uint k = 0; k < buffer[i].nreserved; k++){
            if(buffer[i].entries[k].valid == true){
                nextminRec = i*buffer[0].nreserved+k;
                recid=nextminRec%buffer[i].nreserved;
                break;
            }
        }
        // save the value of minimum record
        if(field == 0 || field == 1)
            minRec = sortvalues[nextminRec];
        else if (field == 2)
            strcpy(minRec2,sortvalues2[nextminRec]);
        else{
            minRec = sortvalues[nextminRec];
            strcpy(minRec2,sortvalues2[nextminRec]);
        }
       // compare each value of the current minimum record with the next records of each segment and find the minimum record
        uint minId = i;
        for (uint j = i + 1; j < numMergingSegs; j++) {
            //find the next valid minimum record
            for(uint k = 0; k < buffer[j].nreserved; k++){
                if(buffer[j].entries[k].valid == true){
                    nextminRec = j*buffer[0].nreserved+k;
                    break;
                }
            }
            //compare records and find minimum
            if(field == 0 || field == 1){
                if (buffer[j].valid && sortvalues[nextminRec]<minRec) {
                    minRec = sortvalues[nextminRec];
                    recid=nextminRec % buffer[j].nreserved;
                    minId = j;
                }
            }
            else if(field == 2){
                if (buffer[j].valid && strcmp(sortvalues2[nextminRec],minRec2) <0 ) {
                    strcpy(minRec2,sortvalues2[nextminRec]);
                    recid=nextminRec % buffer[j].nreserved;
                    minId = j;
                }
            }
            else{
                if(buffer[j].valid && ((sortvalues[nextminRec]<minRec) || (sortvalues[nextminRec]==minRec && strcmp(sortvalues2[nextminRec],minRec2)<0))){
                    minRec = sortvalues[nextminRec];
                    recid=nextminRec % buffer[j].nreserved;
                    minId = j;
                }
            }
        }

        if (lastPass == false) { // minimum record is written to the last block, which is used as output
            last->entries[last->nreserved++] = buffer[minId].entries[recid];
            buffer[minId].entries[recid].valid = false;
        } else {
            for (uint j = 0; j < numMergingSegs; j++) {
                //find the next valid minimum record
                for(uint k = 0; k < buffer[j].nreserved; k++){
                    if(buffer[j].entries[k].valid == true){
                        nextminRec = j*buffer[0].nreserved+k;
                        break;
                    }
                }

                if(block>1){
                    if(field == 0){
                        if(buffer[minId].entries[recid].recid==sortvalues[nextminRec]||buffer[minId].entries[recid].recid==buffer[minId].entries[recid+1].recid)
                            dup++;
                    }
                    else if(field == 1){
                        if(buffer[minId].entries[recid].num==sortvalues[nextminRec]||buffer[minId].entries[recid].num==buffer[minId].entries[recid+1].num)
                            dup++;
                    }
                    else if (field == 2){
                        if (strcmp(buffer[minId].entries[recid].str,sortvalues2[nextminRec])==0||strcmp(buffer[minId].entries[recid].str,buffer[minId].entries[recid+1].str)==0)
                            dup++;
                    }
                    else{
                        if((buffer[minId].entries[recid].num==sortvalues[nextminRec]||buffer[minId].entries[recid].num==buffer[minId].entries[recid+1].num) || (strcmp(buffer[minId].entries[recid].str,sortvalues2[nextminRec])==0||strcmp(buffer[minId].entries[recid].str,buffer[minId].entries[recid+1].str)==0))
                            dup++;
                    }
                }
                else{
                    if(field == 0){
                        if(buffer[minId].entries[recid].recid==buffer[minId].entries[recid+1].recid)
                            dup++;
                    }
                    else if(field == 1){
                        if(buffer[minId].entries[recid].num==buffer[minId].entries[recid+1].num)
                            dup++;
                    }
                    else if (field == 2){
                        if (strcmp(buffer[minId].entries[recid].str,buffer[minId].entries[recid+1].str)==0)
                            dup++;
                    }
                    else{
                        if(buffer[minId].entries[recid].num==buffer[minId].entries[recid+1].num ||strcmp(buffer[minId].entries[recid].str,buffer[minId].entries[recid+1].str)==0)
                            dup++;
                    }
                }
            }
            if(numMergingSegs>1)
                ndups=2;
            else
                ndups=1;

            if (dup>=ndups)
                buffer[minId].entries[recid].valid = false;
            else {
                last->entries[last->nreserved++] = buffer[minId].entries[recid];
                buffer[minId].entries[recid].valid = false;
                (*nunique) += 1;
            }
        }

        // if the last block is full, write it to the outfile
        if (last->nreserved == MAX_RECORDS_PER_BLOCK) {
            write(output, last, sizeof (block_t));
            (*nios) += 1;
            last->blockid += 1;
            outblocks += 1;
            for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++) {
                last->entries[i].valid = false;
            }
            last->nreserved = 0;
        }

        dup =0;
        uint blockId;
        if ((recid+1)-buffer[minId].nreserved==0) { // if we compared all the values of this block
            if (blocksLeft[minId] > 0) {
                if ((lastMerge == true) && minId == numMergingSegs - 1)
                    blockId = firstId + segmentSize * minId + leftSegsize - blocksLeft[minId];
                else
                    blockId = firstId + segmentSize * minId + segmentSize - blocksLeft[minId];
                pread(input, buffer + minId, sizeof (block_t), blockId * sizeof (block_t)); //read the next block
                (*nios) +=1;
                fillarray2(buffer,sortvalues,sortvalues2,field,minId); //fill the sortvalues array with values of the new block
                for(uint k = 0; k < buffer[minId].nreserved; k++)
                    buffer[minId].entries[k].valid =true;
                blocksLeft[minId] -= 1;
                if (buffer[minId].valid == false)
                    numMerge -= 1;
                // check the if first record of the current block equals the last minimum record of the previous block
                if(field == 0){
                    if(buffer[minId].entries[0].recid == minRec){
                            last->entries[--(last->nreserved)].valid = true;
                            (*nunique) -= 1;
                     }
                }
                 else if(field == 1){
                    if(buffer[minId].entries[0].num == minRec){
                            last->entries[--(last->nreserved)].valid = true;
                            (*nunique) -= 1;
                    }
                 }
                 else if(field == 2){
                    if(strcmp(buffer[minId].entries[0].str,minRec2) == 0){
                            last->entries[--(last->nreserved)].valid = true;
                            (*nunique) -= 1;
                    }
                 }
                 else{
                    if(buffer[minId].entries[0].num == minRec || strcmp(buffer[minId].entries[0].str,minRec2) == 0){
                            last->entries[--(last->nreserved)].valid = true;
                            (*nunique) -= 1;
                    }
                 }
            } else {
                buffer[minId].valid = false;
                numMerge -= 1;
            }
        }
        else {
            if (buffer[minId].entries[recid+1].valid == false) {
                buffer[minId].valid = false;
                numMerge -= 1;
            }

        }
    }
    // if merging is over and there are some records on the last block writes them on the output
    if (last->nreserved != 0) {
        write(output, last,last->nreserved * (sizeof (block_t)/(double)MAX_RECORDS_PER_BLOCK));
        (*nios) += 1;
        last->blockid += 1;
        outblocks += 1;
    }

    delete[] minRec2;
}

void dbtproj::EliminateDuplicates (char *infile, unsigned char field, block_t *buffer, unsigned int nmem_blocks, char *outfile, unsigned int *nunique, unsigned int *nios)
{
    *nios = 0;
    struct stat st;
    stat(infile, &st);
    uint size= st.st_size / sizeof (block_t);
    for (uint i = 0; i < nmem_blocks; i++) {
            for (int j = 0; j < MAX_RECORDS_PER_BLOCK; j++)
                    buffer[i].entries[j].valid = false;
        buffer[i].nreserved = 0;
        buffer[i].valid = true;
    }
    //if the file size is smaller or equal to buffer size
    if (size <= nmem_blocks) {
            int input = open(infile, O_RDONLY, S_IRWXU);
            int output = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            read(input, buffer, nmem_blocks * sizeof (block_t));
            (*nios) += nmem_blocks;
            uint recNums = buffer[0].nreserved;
            int counter=0;
            for(uint j = 0; j < nmem_blocks; j++)
                counter= counter + buffer[j].nreserved;
            uint *sortvalues = createarray(buffer,counter);
            char **sortvalues2 = createarray2(buffer,counter);
            fillarray(buffer,nmem_blocks,sortvalues,sortvalues2,field);
            quickSort(buffer,sortvalues,sortvalues2,0,counter,field);
            delete sortvalues;
            for(int k = 0; k < counter; ++k)
                delete [] sortvalues2[k];
            delete [] sortvalues2;
            // all the not unique values of the first block are marked as invalid
            buffer[0].nreserved = 0;
            if(field==0){
                for(uint i=0;i<recNums;i++){
                    if(buffer[0].entries[i].recid==buffer[0].entries[i+1].recid)
                        buffer[0].entries[i].valid=false;
                    else{
                        (*nunique) += 1;
                        buffer[0].nreserved += 1;
                    }
                }
                if(buffer[0].entries[recNums-1].recid==buffer[1].entries[0].recid)
                    buffer[0].entries[recNums-1].valid=false;
             }
             else if(field==1){
                for(uint i=0;i<recNums;i++){
                    if(buffer[0].entries[i].num==buffer[0].entries[i+1].num)
                        buffer[0].entries[i].valid=false;
                    else{
                        (*nunique) += 1;
                        buffer[0].nreserved += 1;
                    }
                }
               if(buffer[0].entries[recNums-1].num==buffer[1].entries[0].num)
                    buffer[0].entries[recNums-1].valid=false;
             }
             else if(field==2){
                for(uint i=0;i<recNums;i++){
                    if(strcmp(buffer[0].entries[i].str,buffer[0].entries[i+1].str)==0)
                        buffer[0].entries[i].valid=false;
                    else{
                        (*nunique) += 1;
                        buffer[0].nreserved += 1;
                    }
                }
                if(strcmp(buffer[0].entries[recNums-1].str,buffer[1].entries[0].str)==0)
                        buffer[0].entries[recNums-1].valid=false;
             }else{
                for(uint i=0;i<recNums;i++){
                    if((buffer[0].entries[i].num==buffer[0].entries[i+1].num) || (strcmp(buffer[0].entries[i].str,buffer[0].entries[i+1].str)==0))
                        buffer[0].entries[i].valid=false;
                    else{
                        (*nunique) += 1;
                        buffer[0].nreserved += 1;
                    }
                }
                if((buffer[0].entries[recNums-1].num==buffer[1].entries[0].num) || (strcmp(buffer[0].entries[recNums-1].str,buffer[1].entries[0].str)==0))
                        buffer[0].entries[recNums-1].valid=false;
             }
             // all the unique values of the first block are moved to the first positions of the block
             for(uint i=0;i<recNums;i++){
                if(buffer[0].entries[i].valid==true && i>=*nunique){
                       for(uint j=0;j<recNums;j++){
                            if(buffer[0].entries[j].valid==false){
                                buffer[0].entries[j]=buffer[0].entries[i];
                                buffer[0].entries[i].valid=false;
                                buffer[0].entries[j].valid=true;
                                break;
                            }
                        }
                }
             }

            // if all the values of the first block are unique write the first block on the output
            if (buffer[0].nreserved == *nunique) {
                write(output, buffer, sizeof (block_t));
                (*nios) += 1;
                for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++)
                   buffer[0].entries[i].valid = false;
                buffer[0].nreserved = 0;
                buffer[0].blockid += 1;
            }

            uint j=1;
            while (buffer[j].valid && j < size) {
                recNums = buffer[j].nreserved;
                //for each record of the current buffer
                for(uint i=0;i<recNums;i++){
                        if(field==0){
                            // compare current record with the next record on the buffer.
                            //if they are not equals we write the current record on the first valid position of the first block
                            if(buffer[j].entries[i].valid && buffer[j].entries[i].recid!=buffer[j].entries[i+1].recid){
                                buffer[0].entries[buffer[0].nreserved++] = buffer[j].entries[i];
                                (*nunique) += 1;
                            }
                            // special check between  last record of the current block and  first record of the next block
                            if((i==recNums-1) && (buffer[j+1].entries[0].valid) && (buffer[j].entries[recNums-1].recid==buffer[j+1].entries[0].recid)){
                                buffer[0].nreserved--;
                                 buffer[0].entries[buffer[0].nreserved].valid = false;
                                (*nunique) -= 1;
                            }
                        }
                        else if(field==1){
                            if(buffer[j].entries[i].valid && (buffer[j].entries[i].num!=buffer[j].entries[i+1].num)){
                                buffer[0].entries[buffer[0].nreserved++] = buffer[j].entries[i];
                                (*nunique) += 1;
                            }
                            if((i==recNums-1) && (buffer[j+1].entries[0].valid) && (buffer[j].entries[recNums-1].num==buffer[j+1].entries[0].num)){
                                 buffer[0].nreserved--;
                                 buffer[0].entries[buffer[0].nreserved].valid = false;
                                (*nunique) -= 1;
                            }
                        }
                        else if(field==2){
                            if(buffer[j].entries[i].valid && strcmp(buffer[j].entries[i].str,buffer[j].entries[i+1].str)!=0){
                                buffer[0].entries[buffer[0].nreserved++] = buffer[j].entries[i];
                                (*nunique) += 1;
                            }
                            if((i==recNums-1) && (buffer[j+1].entries[0].valid) && strcmp(buffer[j].entries[recNums-1].str,buffer[j+1].entries[0].str)==0){
                                buffer[0].nreserved--;
                                 buffer[0].entries[buffer[0].nreserved].valid = false;
                                (*nunique) -= 1;
                            }
                        }
                        else{
                            if(buffer[j].entries[i].valid && ((buffer[j].entries[i].num!=buffer[j].entries[i+1].num) && (strcmp(buffer[j].entries[i].str,buffer[j].entries[i+1].str)!=0))){
                                buffer[0].entries[buffer[0].nreserved++] = buffer[j].entries[i];
                                (*nunique) += 1;
                            }
                            if((i==recNums-1) && (buffer[j+1].entries[0].valid) &&((buffer[j].entries[recNums-1].num==buffer[j+1].entries[0].num) || strcmp(buffer[j].entries[recNums-1].str,buffer[j+1].entries[0].str)==0)){
                                --(buffer[0].nreserved) ;
                                (*nunique) -= 1;
                            }
                        }
                        // if all the positions of the first block are full write the first block on the output
                        if (buffer[0].nreserved == MAX_RECORDS_PER_BLOCK) {
                            write(output, buffer, 1 * sizeof (block_t));
                            (*nios) += 1;
                            for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++)
                               buffer[0].entries[i].valid = false;
                            buffer[0].nreserved = 0;
                            buffer[0].blockid += 1;
                        }
                 }
                 j++;
            }
            //in the end, if there are some records on the first block write it on the output
            if (buffer[0].nreserved != 0) {
                write(output, buffer, buffer[0].nreserved * (sizeof (block_t)/(double)MAX_RECORDS_PER_BLOCK));
                (*nios) += 1;
            }
            close(output);
    } else { // if the file size is larger than the buffer then sort it using mergesort

        char tmp1[] = ".t1";
        char tmp2[] = ".t2";
        int input = open(infile, O_RDONLY, S_IRWXU);
        int output = open(tmp1, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

        uint fullbuffers = size / nmem_blocks; // number of segments that fill the buffer
        uint leftbuffers = size % nmem_blocks; // left blocks that doen't fill the buffer
        uint sortedbuffers = 0;
        uint bufferblocks = nmem_blocks;
        // sort each segment and write it to the temp file 1
        for (uint i = 0; i <= fullbuffers; i++) {
            if (fullbuffers == i) {
                if (leftbuffers != 0)
                    bufferblocks = leftbuffers;
                else
                    break;
            }
            read(input, buffer, bufferblocks * sizeof (block_t));
             (*nios) += bufferblocks;
            int counter=0;
            for(uint j = 0; j < bufferblocks; j++)
                counter= counter + buffer[j].nreserved; // find the valid number of records in the buffer
             uint *sortvalues = createarray(buffer,counter); //create an array of integers with the size of buffer (for field 0,1,3)
             char **sortvalues2 = createarray2(buffer,counter); //create an array of strings(for field 2,3)
             fillarray(buffer,bufferblocks,sortvalues,sortvalues2,field); //fill the array with the values of each record
             quickSort(buffer,sortvalues,sortvalues2,0,counter,field); //sort the array and at the same time sort the buffer
             delete sortvalues;
             for(int k = 0; k < counter; ++k)
                delete [] sortvalues2[k];
            delete [] sortvalues2;
             write(output, buffer, bufferblocks * sizeof (block_t));
            (*nios) += bufferblocks;
            sortedbuffers += 1;
        }
        close(input);
        close(output);

        uint numleftbuffers; //blocks of last segment
        if (leftbuffers == 0)
            numleftbuffers = nmem_blocks;
        else
            numleftbuffers = leftbuffers;
        bufferblocks = nmem_blocks;
        buffer[nmem_blocks - 1].valid = true;
        while (sortedbuffers > 1) {
            input = open(tmp1, O_RDONLY, S_IRWXU);
            output = open(tmp2, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            uint *blocksLeft = new unsigned int[(nmem_blocks - 1) * sizeof (uint)]; //number of blocks left of a segment
            uint tmpSortedbuffers = 0;
            uint numMergingSegs = nmem_blocks - 1;
            uint mergingSegs = sortedbuffers / (nmem_blocks - 1); // number of merges that fill the buffer
            uint leftMergingSegs = sortedbuffers % (nmem_blocks - 1); // number of merges that segment doesn't fill the buffer
            bool lastMerge = false; //check if it's the last merge

            for (uint j = 0; j <= mergingSegs; j++) {
                if ((j == mergingSegs - 1) && (leftMergingSegs == 0))
                    lastMerge = true;
                else if (j == mergingSegs) {
                    if (leftMergingSegs != 0) {
                        numMergingSegs = leftMergingSegs;
                        lastMerge = true;
                    } else
                        break;
                }
                int counter=0;
                // take the first block of each segment to merge on the buffer
                for (uint i = 0; i < numMergingSegs; i++) {
                    pread(input, buffer + i, sizeof (block_t),((j * (nmem_blocks - 1) * bufferblocks) + i * bufferblocks) * sizeof (block_t));
                    (*nios) +=1;
                    counter= counter + buffer[i].nreserved;
                    blocksLeft[i] = bufferblocks - 1;
                }
                uint *sortvalues = createarray(buffer,counter);
                char **sortvalues2 = createarray2(buffer,counter);
                fillarray(buffer,numMergingSegs,sortvalues,sortvalues2,field);

                if (lastMerge == true) // we get the number of  blocks of the last segment
                    blocksLeft[numMergingSegs - 1] = numleftbuffers - 1;
                bool last = sortedbuffers <= (nmem_blocks - 1);
                eliminateDuplicatesMerging(input, output, buffer, nmem_blocks - 1, numMergingSegs, blocksLeft, bufferblocks, j * (nmem_blocks - 1) * bufferblocks, field, last, lastMerge, sortvalues, sortvalues2, nunique, nios);
                tmpSortedbuffers += 1;
                delete sortvalues;
                for(int k = 0; k < counter; ++k)
                    delete [] sortvalues2[k];
                delete [] sortvalues2;
            }
            delete[] blocksLeft;
            if (leftMergingSegs == 0)
                numleftbuffers = (nmem_blocks - 2)*bufferblocks + numleftbuffers;
            else
                numleftbuffers = (leftMergingSegs - 1) * bufferblocks + numleftbuffers;
            bufferblocks = bufferblocks * (nmem_blocks - 1);
            sortedbuffers = tmpSortedbuffers;
            // swaps the files for the next pass
            char tmp = tmp1[2];
            tmp1[2] = tmp2[2];
            tmp2[2] = tmp;
            close(input);
            close(output);
        }
        rename(tmp1, outfile); // rename outfile
        remove(tmp2);
        remove(tmp1);
    }
}

//compare records according to field
int compare(record_t rec1, record_t rec2, unsigned char field) {
        if(field == 0){
            if (rec1.recid < rec2.recid)
                return -1;
            else if (rec1.recid > rec2.recid)
                return 1;
            else
                return 0;
        }
        else if(field == 1){
            if (rec1.num < rec2.num)
                return -1;
            else if (rec1.num > rec2.num)
                return 1;
            else
                return 0;
        }
        else if(field == 2){
            if (strcmp(rec1.str, rec2.str) < 0)
                return -1;
            else if (strcmp(rec1.str, rec2.str) > 0)
                return 1;
            else
                return 0;
        }
        else{
            if ((rec1.num < rec2.num || (rec1.num == rec2.num && strcmp(rec1.str, rec2.str) < 0)))
                return -1;
            else if ((rec1.num > rec2.num || (rec1.num == rec2.num && strcmp(rec1.str, rec2.str) > 0)))
                return 1;
            else
                return 0;
        }
}

void dbtproj::MergeJoin (char *infile1, char *infile2, unsigned char field, block_t *buffer, unsigned int nmem_blocks, char *outfile, unsigned int *nres, unsigned int *nios)
{
        *nios = 0;
        uint sizeMemory;
        char tmp1[] = ".tmp1";
        char tmp2[] = ".tmp2";
        for (uint i = 0; i < nmem_blocks; i++) { //empty buffer
                for (int j = 0; j < MAX_RECORDS_PER_BLOCK; j++)
                        buffer[i].entries[j].valid = false;
            buffer[i].nreserved = 0;
            buffer[i].valid = true;
        }
        struct stat st;
        stat(infile1, &st);
        uint size1= st.st_size / sizeof (block_t); //number of blocks of file 1
        stat(infile2, &st);
        uint size2= st.st_size / sizeof (block_t); //number of blocks of file 2

        uint d1=0, d2=0, ios;
        MergeSort(infile1, field, buffer, nmem_blocks, tmp1, &d1, &d2, &ios); //file 1 sorted using mergesort
        (*nios) += ios;
        d1=0, d2=0;
        MergeSort(infile2, field, buffer, nmem_blocks, tmp2, &d1, &d2, &ios); //file 2 sorted using mergesort
        (*nios) += ios;
        int out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

        if (size1 > size2) { // if file 1 is bigger that file 2 switch tmps and sizes
                uint tmp = size1;
                size1 = size2;
                size2 = tmp;
                tmp1[4] = '2';
                tmp2[4] = '1';
        }

        if (size1 < nmem_blocks - 2)
                sizeMemory = size1; // number of blocks in memory
        else
                sizeMemory = nmem_blocks - 2;

        int in1 = open(tmp1, O_RDONLY, S_IRWXU);
        int in2 = open(tmp2, O_RDONLY, S_IRWXU);

        block_t *blockforBig = buffer + nmem_blocks - 2; //pointer to a block before the last block of buffer where a block of the bigger file will be loaded each time
        block_t *last = buffer + nmem_blocks - 1; //pointer to last block of buffer as output
        last->blockid = 0;

        read(in1, buffer, sizeMemory * sizeof (block_t)); //read as many blocks of the smaller file as the sizeMemory is
        (*nios) += sizeMemory;
        read(in2, blockforBig, sizeof (block_t)); // read one block of the bigger file
        (*nios) += 1;

        record_t temp = buffer[0].entries[0]; //get the first record of the buffer
        uint blockschecked = 1; // check how many blocks of the bigger file have been loaded
        uint tempblockid =0;
        uint firstblockid = 0; // first block id of the smaller file blocks that have beed load in buffer
        uint lastblockid = sizeMemory - 1; //last block id of the smaller file blocks that have beed load in buffer
        uint recordid=0; //current record id of buffer
        uint blockid =0; //current block id of buffer
        bool b = 0;
        while (b == 0) {
                //for each record of the the fileBuffer
                for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++) {
                        record_t rec = blockforBig->entries[i]; //get the next record of the block
                        if (rec.valid == false) { // if this record is unvalid, finish while loop
                            b = 1;
                            break;
                        }
                        // if records of the 2 files equals
                        if (compare(rec, temp, field) == 0) {
                                if (tempblockid < firstblockid) { //block id is not in the buffer currently
                                    uint numblocks;
                                    if (firstblockid - tempblockid <= sizeMemory)
                                        numblocks = firstblockid - tempblockid;
                                    else
                                        numblocks = sizeMemory;
                                    for (uint i = 0; i < numblocks; i++) {// reload the block as well as the others after it
                                        pread(in1, buffer + (tempblockid + i) % sizeMemory, sizeof (block_t), (tempblockid + i) * sizeof (block_t));
                                        (*nios) += 1;
                                    }
                                    firstblockid = tempblockid;
                                    lastblockid = firstblockid + sizeMemory - 1;
                                }
                                blockid = tempblockid % sizeMemory;
                                recordid = 0;
                        }
                        //looking for a record of the smaller file, with higher or equal value with rec
                        while (compare(buffer[blockid].entries[recordid], rec, field) < 0) {
                            // update record id and block id
                            if (recordid < MAX_RECORDS_PER_BLOCK - 1)
                                 recordid+=1;
                            else{
                                recordid=0;
                                blockid+=1;
                            }
                            //if we are in the first record of a block
                            if (recordid == 0) {
                                blockid = blockid % sizeMemory;
                                if (blockid == firstblockid % sizeMemory) {
                                    if (lastblockid < size1-1) { // if we have loaded less blocks of the smaller file than they exists
                                        pread(in1, buffer + firstblockid % sizeMemory, sizeof (block_t), (lastblockid + 1) * sizeof (block_t));
                                        (*nios) += 1;
                                        firstblockid += 1;
                                        lastblockid += 1;
                                    } else { //finish while loop
                                        b = 1;
                                        break;
                                    }
                                }
                            }
                            // if this record is unvalid, finish while loop
                            if (buffer[blockid].entries[recordid].valid == false) {
                                b = 1;
                                break;
                            }
                        }

                        temp = buffer[blockid].entries[recordid];
                        if (blockid >= firstblockid % sizeMemory)
                            tempblockid = firstblockid + blockid - firstblockid % sizeMemory;
                        else
                            tempblockid = firstblockid + blockid + sizeMemory - firstblockid % sizeMemory;

                        //  all the records from the other file equals to rec in field value are written to the output along with rec each time.
                        while (compare(buffer[blockid].entries[recordid], rec, field) == 0) {
                            last->entries[last->nreserved++] = rec;
                            last->entries[last->nreserved++] = buffer[blockid].entries[recordid];
                            (*nres) += 1;

                            // if last block become full, writes it to the output
                            if (last->nreserved == MAX_RECORDS_PER_BLOCK) {
                                write(out, last, sizeof (block_t));
                                (*nios) += 1;
                                for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++) //empty last block
                                    last->entries[i].valid = false;
                                last->nreserved = 0;
                                last->blockid += 1;
                            }
                            // update record id and block id
                            if (recordid < MAX_RECORDS_PER_BLOCK - 1)
                                 recordid+=1;
                            else{
                                recordid=0;
                                blockid+=1;
                            }
                            //if we are in the first record of a block
                            if (recordid == 0) {
                                blockid = blockid % sizeMemory;
                                if (blockid == firstblockid % sizeMemory) {
                                    if (lastblockid < size1-1) { // if we have loaded less blocks of the smaller file than they exists
                                        pread(in1, buffer + firstblockid % sizeMemory, sizeof (block_t), (lastblockid + 1) * sizeof (block_t)); // loads the next block of smaller file
                                        (*nios) += 1;
                                        firstblockid += 1;
                                        lastblockid += 1;
                                    } else {
                                        if (blockid == 0)
                                            blockid = sizeMemory - 1;
                                        else
                                            blockid -= 1;
                                        recordid = MAX_RECORDS_PER_BLOCK - 1;
                                        break;
                                    }
                                }
                            }
                            // if we reach in the end of the block
                            if (buffer[blockid].entries[recordid].valid == false) {
                                if (recordid > 0)
                                    blockid-=1;
                                else if (blockid > 0) {
                                    recordid = MAX_RECORDS_PER_BLOCK - 1;
                                    blockid-= 1;
                                }
                                break;
                            }
                        }
                }
                if (b == 1)
                    break;
                // if there are more blocks of the bigger file to be loaded
                if (blockschecked < size2) {
                    read(in2, blockforBig, sizeof (block_t));
                    (*nios) += 1;
                    blockschecked += 1;
                } else
                    b = 1; //finish while loop
        }
        // if the are some record in the last block writes them to the outfile
        if (last->nreserved != 0) {
                write(out, last, last->nreserved*(sizeof (block_t)/(double)MAX_RECORDS_PER_BLOCK));
                (*nios) += 1;
        }
        close(in1);
        close(in2);
        remove(tmp1);
        remove(tmp2);
        close(out);
}

// hash function for integers
uint hashInt(uint x,uint mod) {
    x = (x + 0x7ed55d16) + (x << 12);
    x= (x^0xc761c23c) ^ (x >> 19);
    x = (x + 0x165667b1) + (x << 5);
    x = (x + 0xd3a2646c) ^ (x << 9);
    x = (x + 0xfd7046c5) + (x << 3);
    x = (x^0xb55a4f09) ^ (x >> 16);
    return x % mod;
}

// hash function for strings
uint hashString(char *str, uint mod) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % mod;
}

// get value of hash function
uint hashing(record_t rec, uint mod, unsigned char field) {
    uint x;
    if(field==0){
            x=hashInt(rec.recid, mod);
            return x;
    }
    else if(field==1){
            x=hashInt(rec.num,mod);
            return x;
    }
     else if(field==2){
            x=hashString(rec.str, mod);
            return x;
     }
      else{
           x=hashInt(rec.num + hashString(rec.str, mod), mod);
           return x;
    }
}


void dbtproj::HashJoin (char *infile1, char *infile2, unsigned char field, block_t *buffer, unsigned int nmem_blocks, char *outfile, unsigned int *nres, unsigned int *nios)
{
    (*nres) = 0;
    (*nios) = 0;
    uint fullBlock,remainingBlock,sizee;
    int in1,in2;
    for (uint i = 0; i < nmem_blocks; i++) { //empties buffer
            for (int j = 0; j < MAX_RECORDS_PER_BLOCK; j++)
                    buffer[i].entries[j].valid = false;
        buffer[i].nreserved = 0;
        buffer[i].valid = true;
    }
    struct stat st;
    stat(infile1, &st);
    uint size1= st.st_size / sizeof (block_t); //number of blocks of file 1
    stat(infile2, &st);
    uint size2= st.st_size / sizeof (block_t); //number of blocks of file 1

    if (size1 <= size2) { //we need the smallest file to fill the hash
        fullBlock = size1 / (nmem_blocks-2); // find the number of segments that fill the nmem_blocks-2 blocks completely
        remainingBlock = size1 % (nmem_blocks-2); // find if there is a segment left that can't fill the nmem_blocks-2 blocks completely
        in1 = open(infile1, O_RDONLY, S_IRWXU);
    } else {
        fullBlock = size2 / (nmem_blocks-2);
        remainingBlock = size2 % (nmem_blocks-2);
        in1 = open(infile2, O_RDONLY, S_IRWXU);
    }
    if(remainingBlock>0)
         fullBlock +=remainingBlock; // find the number of merges we will need
    int out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    block_t *blockforBig = buffer+nmem_blocks-2; //pointer to a block before the last block of buffer where a block of the bigger file will be loaded each time
    block_t *last = buffer+nmem_blocks-1; //pointer to last block of buffer as output
    last->blockid = 0;

    while(fullBlock!=0){
        uint size = (nmem_blocks-2) * MAX_RECORDS_PER_BLOCK;
        linkedList **hashtable = (linkedList**) malloc(size * sizeof (linkedList*)); //create a hashtable with the size of the number of records of the smaller file in buffer
        for (uint i = 0; i < size; i++)
                hashtable[i] = NULL;
        read(in1, buffer, (nmem_blocks - 2)*sizeof (block_t));
        (*nios) += (nmem_blocks - 2);

        record_t temp;
        temp.rdummy1 =0; //block id
        temp.rdummy2=0; //record id
        int counter=0;
        //for each record of the smaller file loaded in the buffer
        while (counter <= ((nmem_blocks-2) * MAX_RECORDS_PER_BLOCK - 1)) {
            record_t rec = buffer[temp.rdummy1].entries[temp.rdummy2];
            if (rec.valid == true) {
                uint number = hashing(rec, size, field); //get the number of the position the record will have in the hashtable
                linkedList *rec2 = (linkedList*) malloc(sizeof (linkedList));
                //set rec to the position of hashtable we found and create a pointer for the next record in the same position of hashtable
                rec2->current = temp;
                rec2->next = hashtable[number];
                hashtable[number] = rec2;
            }
            // update record id and block id
            if (temp.rdummy2 < MAX_RECORDS_PER_BLOCK - 1)
                        temp.rdummy2+=1;
            else{
                        temp.rdummy2=0;
                        temp.rdummy1+=1;
            }
            counter++;
        }

        if (size1<=size2) {
            in2 = open(infile2, O_RDONLY, S_IRWXU);
            sizee=size2;
        } else {
            in2 = open(infile1, O_RDONLY, S_IRWXU);
            sizee=size1;
        }
        //for each block of the bigger file loaded in the buffer
        for (uint i = 0; i < sizee; i++) {
            read(in2, blockforBig, sizeof (block_t));
            (*nios) += 1;
            //for each record of the bigger file block loaded in the buffer
            for (int j = 0; j < MAX_RECORDS_PER_BLOCK; j++) {
                record_t rec = blockforBig->entries[j];
                if (rec.valid == true) {
                    uint number = hashing(rec, size, field); //get the number of the position the record will have in the hashtable
                    linkedList *tmp2 = hashtable[number];
                    //for each record in this posotion
                    while (tmp2) {
                        record_t tmp = buffer[tmp2->current.rdummy1].entries[tmp2->current.rdummy2];
                        if (compare(rec,tmp,field) == 0) { // if they have the same field value, writes them to the outfile
                            last->entries[last->nreserved++] = rec;
                            last->entries[last->nreserved++] = tmp;
                            (*nres) += 1;
                            // if last block become full, writes it to the output
                            if (last->nreserved == MAX_RECORDS_PER_BLOCK) {
                                    write(out, last, sizeof (block_t));
                                    (*nios) += 1;
                                for (int i = 0; i < MAX_RECORDS_PER_BLOCK; i++)//empty last block
                                        last->entries[i].valid = false;
                                last->nreserved = 0;
                                last->blockid += 1;
                            }
                        }
                        tmp2 = tmp2->next;
                    }
                }
            }
        }
        for(int i = 0; i < (nmem_blocks-2)*MAX_RECORDS_PER_BLOCK; ++i) {
                   linkedList* hashT = hashtable[i];
                   free(hashT);
        }
        fullBlock--;
        if(fullBlock==0){
                close(in1);
                close(in2);
        }

    }
    // if the are some record in the last block writes them to the outfile
    if (last->nreserved != 0) {
            write(out, last, last->nreserved*(sizeof (block_t)/(double)MAX_RECORDS_PER_BLOCK));
            (*nios) += 1;
    }
    close(out);

}
