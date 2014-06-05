#include "stdio.h"
#include "permutations.h"
#include "time.h"

#define COUNT 5

void print_array(int*, int);

// total 1932053504 permutations   93.411995
// Speed = 20.683142 millions per/secthe end

// total 479001600 permutations    7.203000
// Speed = 66.500290 millions per/secthe end

void main()
{
	int arr[COUNT], dirs[COUNT];
	int res = 1, i;
	unsigned int cnt;
	float diff;
	clock_t t1, t2;

	for(i=0; i<COUNT; i++)
	{
		arr[i] = i + 1;
		dirs[i] = -1;
	}

	printf("Permutations\n");

	//tests();

	cnt = 0;

	t1 = clock();
	while( res == 1 )
	{
		if( cnt % 1 == 0 )
		{
			//printf("%d\t", cnt);
			print_array(arr, COUNT, "", "\n");
			//print_array(dirs, COUNT, "\t", "\n\n");

		}

		//res = permutation(arr, COUNT);
		res = permutation_recur(arr, dirs, COUNT);

		cnt ++ ;
	}
	print_array(arr, COUNT, "", "\n");
	t2 = clock();

	diff = (((float)t2 - (float)t1) / 1000000.0F ) * 1000;  
	printf("\n total %d permutations\t %f\n", cnt, diff);
	printf("Speed = %f millions per/sec", cnt/diff/1000000);
	printf("the end \n");
	getchar();
}

int _assert_equals(int a, int b, char *msg)
{
	if( a != b )
	{
		printf("%s\n", msg);
		return 0;
	}
	return 1;
}

int _assert_array_equals(int *arr1, int *arr2, int len, char *msg)
{ //сравнивает два массива. Если НЕ равны, то выводит сообщение msg
	int i;
	for(i=0; i<len; i++)
	{
		if( arr1[i] != arr2[i] )
		{
			printf("%s\n", msg);
			return 0;
		}
	}

	return 1; //все совпало
}


void print_array(int* arr, int len, char *pref, char* suff)
{
	int i;
	printf("%s", pref);
	for(i=0; i<len; i++)
		printf("%3d", arr[i]);
	if( suff != NULL )
		printf(suff);
}
