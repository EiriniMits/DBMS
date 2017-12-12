# DBMS
This project was created for academic purposes on the subject of 'Database Technology' at the Aristotle University of Thessaloniki.

This project contains basic functions of a DBMS (External Merge sort, Duplicate Elimination, Merge Join, Hash Join) that are designed to work in real-life and extreme circumstances (Huge input data, extremely low available memory).

## Examples

**Input:**
```
- N: number of blocks in the file, N1: number of blocks in the file1, N2: number of blocks in the file2
- B: number of blocks in the memory buffer
- field: which field will be used for sorting: 0 is for recid, 1 is for num, 2 is for str and 3 is for both num and str
```
**Output:**
```
- nsorted_segs: number of sorted segments produced 
- npasses: number of passes required for sorting 
- nios: number of IOs performed 
```
## External Merge sort
**-Input File-**

<img width="600" alt="screen shot 2017-11-16 at 8 23 39 pm" src="https://user-images.githubusercontent.com/16197563/32908438-401330ac-cb0c-11e7-8db2-d360fb78c0a6.png">

```
1. N = 10, B = 3, MAX_RECORDS_PER_BLOCK = 10, field = 1
```
**-Output File-**

<img width="600" alt="screen shot 2017-11-16 at 8 23 50 pm" src="https://user-images.githubusercontent.com/16197563/32908441-421c183c-cb0c-11e7-8f92-8cdb169307b3.png">

```
2. N = 10, B = 3, MAX_RECORDS_PER_BLOCK = 10, field = 2
```
**-Output File-**

<img width="600" alt="screen shot 2017-11-16 at 8 24 17 pm" src="https://user-images.githubusercontent.com/16197563/32908444-42b4d658-cb0c-11e7-822b-c77b76b48018.png">

## Duplicate Elimination
**-Input File-**

<img width="600" alt="screen shot 2017-11-16 at 8 31 02 pm" src="https://user-images.githubusercontent.com/16197563/32908749-2d11e10a-cb0d-11e7-8dc7-dea55a2e6e7f.png">

```
1. N = 10, B = 4, MAX_RECORDS_PER_BLOCK = 10, field = 1
```
**-Output File-**

<img width="350" alt="screen shot 2017-11-16 at 8 30 48 pm" src="https://user-images.githubusercontent.com/16197563/32908750-2d3444ac-cb0d-11e7-8d16-b711f760c211.png">

```
2. N = 10, B = 4, MAX_RECORDS_PER_BLOCK = 10, field = 2
```
**-Output File-**

<img width="600" alt="screen shot 2017-11-16 at 8 24 17 pm" src="https://user-images.githubusercontent.com/16197563/32908751-2d66c6de-cb0d-11e7-82fa-4f407d405511.png">

## Merge Join
**-Input File 1 & Input File 2-**

<img width="600" alt="screen shot 2017-11-16 at 8 39 18 pm" src="https://user-images.githubusercontent.com/16197563/32909134-6debfcbe-cb0e-11e7-88f0-127f0379714b.png">
<img width="600" alt="screen shot 2017-11-16 at 8 39 28 pm" src="https://user-images.githubusercontent.com/16197563/32909135-6ec31974-cb0e-11e7-8677-27532fbc7457.png">

```
N1 = 10, N2 = 10, B = 3, MAX_RECORDS_PER_BLOCK = 10, field = 1
```
**-Output File-**

<img width="500" alt="screen shot 2017-11-16 at 8 40 09 pm" src="https://user-images.githubusercontent.com/16197563/32909136-6fec2430-cb0e-11e7-8ec8-143a7c6ddf25.png">

## Hash Join
**-Input File 1 & Input File 2-**

<img width="600" alt="screen shot 2017-11-16 at 8 46 06 pm" src="https://user-images.githubusercontent.com/16197563/32909486-4e4d6946-cb0f-11e7-8c75-1d37c485fe7b.png">
<img width="600" alt="screen shot 2017-11-16 at 8 46 29 pm" src="https://user-images.githubusercontent.com/16197563/32909518-5e386efa-cb0f-11e7-8992-c21d19681d40.png">

```
N1 = 10, N2 = 10, B = 3, MAX_RECORDS_PER_BLOCK = 10, field = 1
```
**-Output File-**

<img width="600" alt="screen shot 2017-11-16 at 8 46 37 pm" src="https://user-images.githubusercontent.com/16197563/32909516-5e11f05e-cb0f-11e7-8d51-d3693b8da0ef.png">

## More examples in bigger input data sets

### MergeSort:

<img width="500" alt="screen shot 2017-11-16 at 8 53 48 pm" src="https://user-images.githubusercontent.com/16197563/32909851-62d40bbc-cb10-11e7-9b59-23dac6bafcc4.png">

### EliminateDuplicates:

<img width="450" alt="screen shot 2017-11-16 at 8 54 00 pm" src="https://user-images.githubusercontent.com/16197563/32909863-6940aa00-cb10-11e7-98cb-f1acd4dd0fc2.png">

### MergeJoin & ΗashJoin:

<img width="500" alt="screen shot 2017-11-16 at 8 54 12 pm" src="https://user-images.githubusercontent.com/16197563/32909861-68c2dad0-cb10-11e7-85ff-2498c5d5b966.png">
<img width="500" alt="screen shot 2017-11-16 at 8 54 21 pm" src="https://user-images.githubusercontent.com/16197563/32909860-68379380-cb10-11e7-9445-5feab0268456.png">
