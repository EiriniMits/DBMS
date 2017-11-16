#include "dbtproj.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
using namespace std;

# Eirini Mitsopoulou

void createfile(char *filename,int nblocks) {
        FILE *out;
        block_t block;
        record_t record;
        uint recid = 0;
        char s[5]= {0};
        static const char alphanum[] ="abcdefghijklmnopqrstuvwxyz";
        out = fopen(filename, "w");
        for (int b=0; b<nblocks; ++b) { // for each block
            block.blockid = b;
            for (int r=0; r<MAX_RECORDS_PER_BLOCK; ++r) { // for each record
                record.recid = recid++;
                record.num = rand() % 100;
                for (int i = 0; i < 4; ++i)
                    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
                strcpy(record.str,s);
                record.valid = true;
                memcpy(&block.entries[r], &record, sizeof(record_t)); // copy record to block
            }
            block.valid = true;
            block.nreserved = MAX_RECORDS_PER_BLOCK;
            fwrite(&block, 1, sizeof(block_t), out);	// write the block to the outfile
        }
        fclose(out);
}

void printfile(char *filename,int nblocks){
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

int main(int argc, char** argv) {

	uint nblocks1 = 200;	// number of blocks in the file1
	uint nblocks2 = 1000;   // number of blocks in the file2
	uint nmem_blocks = 6; // number of blocks in the buffer
	block_t* buffer = (block_t*) malloc(nmem_blocks * sizeof (block_t));
	char file1[] = "f1.bin";
	char file2[] = "f2.bin";
	char outfile[] = "fout.bin";
	uint nsorted_segs=0;
	uint npasses=0;
	uint nios=0;
	uint nunique=0;
	uint nres=0;
	dbtproj dbt;

    createfile(file1,nblocks1); //create file1
    createfile(file2,nblocks2); //create file2

    cout<<"-------------------File1-------------------"<<endl;
    printfile(file1,nblocks1);
    cout<<"-------------------File2-------------------"<<endl;
    printfile(file2,nblocks2);

    dbt.MergeSort(file1, 1, buffer, nmem_blocks, outfile, &nsorted_segs, &npasses, &nios);
    cout<<"-----------------MergeSort-----------------"<<endl;
    printfile(outfile,nblocks1);
    cout<<"-------------------------------------------"<<endl;
    cout<<"npasses: "<<npasses<<",  nsorted_segs: "<< nsorted_segs<<", nios: "<<nios<<endl;

    dbt.EliminateDuplicates(file1, 2, buffer, nmem_blocks, outfile, &nunique, &nios);
    cout<<"------------EliminateDuplicates------------"<<endl;
    printfile(outfile,nblocks1);
    cout<<"-------------------------------------------"<<endl;
    cout<<"nunique: "<<nunique<<", nios: "<<nios<<endl;


    dbt.MergeJoin(file1, file2, 1, buffer, nmem_blocks, outfile, &nres, &nios);
    cout<<"-----------------MergeJoin-----------------"<<endl;
    printfile(outfile,nblocks1*nblocks2);
    cout<<"-------------------------------------------"<<endl;
    cout<<"nres: "<<nres<<", nios: "<<nios<<endl;


    dbt.HashJoin(file1, file2, 1, buffer, nmem_blocks, outfile, &nres, &nios);
    cout<<"-----------------HashJoin-----------------"<<endl;
    printfile(outfile,nblocks1*nblocks2);
    cout<<"------------------------------------------"<<endl;
    cout<<"nres: "<<nres<<", nios: "<<nios<<endl;

	return 0;
}
