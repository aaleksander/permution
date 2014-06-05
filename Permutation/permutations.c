#include "stdio.h"

#define PERM_LEFT -1
#define PERM_RIGHT 1

void qs(int*, int, int);

int permutation(int* arr, int cnt)
{
	//алгоритм взят отсюда: http://rain.ifmo.ru/cat/view.php/vis/combinations/permutations-2000
	int i, k, min=-1, tmp, s, arri, arrk, *t;
	int res = 0, f;
	if( cnt <= 2 )
		return 0;
	for(i=cnt - 2; i >= 0; i-- )
	{
		if( *(arr + i) < *(arr + i + 1) )
		{			
			res = 1;
			//ищем минимальный элемент справа
			arri = arr[i];
			for(k=i + 1; k<cnt; k++)
			{
				arrk = arr[k];
				if( min == -1 && arrk > arri )
				{
					min = k;
					continue;
				}
				if( arrk > arri && arrk < arr[min] ) //нашли минимальный элемент
				{
					min = k;
				}
			}
			break;
		}
	}

	//меняем местами
	if( min != -1 )
	{
		tmp = arr[i];
		arr[i] = arr[min];
		arr[min] = tmp;

		//сортируем правую часть по возрастанию (пузырьком)
		//qs(arr, i + 1, cnt - 1);

		f = 1;
		while( f == 1 )
		{
			f = 0;
			t = arr + i + 1;
			for(s=i + 1;s<cnt - 1; s++)
			{
				if( *t > *(t + 1) )
				{
					f = 1;
					tmp = *t;
					*t = *(t + 1);
					*(t + 1) = tmp;
				}
				t++;
			}
		}
		return 1;
	}

	return 0;
}

void qs(int* s_arr, int first, int last)
{
    int i = first, j = last, x = s_arr[(first + last) / 2], tmp;
 
    do {
        while (s_arr[i] < x) i++;
        while (s_arr[j] > x) j--;
 
        if(i <= j) {
            if (i < j)
			{
				tmp = s_arr[i];
				s_arr[i] = s_arr[j];
				s_arr[j] = tmp;
			}
            i++;
            j--;
        }
    } while (i <= j);
 
    if (i < last)
        qs(s_arr, i, last);
    if (first < j)
        qs(s_arr, first,j);
}

int get_max(int* arr, int len, int less_then)
{//возвращает индекс максимального элемента в массиве, но меньше, чем less_then
	int i, res = -1;
	for(i = 0;  i<len; i++)
	{
		if( arr[res] < arr[i] && arr[i] < less_then)
			res = i;
	}

	return res;
}

int can_move_right(int *arr, int len, int elem)
{//возвращает 1, если элемент нельзя сдвинуть вправо
	if( elem >= len - 1 )
		return 0;
	if( arr[elem + 1] > arr[elem] )
		return 0;
	return 1;
}

int can_move_left(int *arr, int len, int elem)
{//возвращает 1, если элемент нельзя сдвинуть вправо
	if( elem == 0 )
		return 0;
	if( arr[elem - 1] > arr[elem] )
		return 0;
	return 1;
}

int get_movable(int *arr, int *dirs, int len)
{//возвращает индекс элемента, который можно переставить
	int max_i = -1, res = -1;

	//ищем максимальный элемент
	int d=1;
	while( max_i == -1 )
	{
		max_i = get_max(arr, len, len + d);
		if(max_i == -1)
			break;

		if( dirs[max_i] == PERM_RIGHT && can_move_right(arr, len, max_i) == 0 ) //вправо двигаться нельзя
		{
			dirs[max_i] = PERM_LEFT;
			max_i = -1;
			d--;
		}

		if( dirs[max_i] == PERM_LEFT && can_move_left(arr, len, max_i) == 0 ) //вправо двигаться нельзя
		{
			dirs[max_i] = PERM_RIGHT;
			max_i = -1;
			d--;
		}


		/*if( (max_i == 0 && dirs[max_i] == PERM_LEFT) //уперлись влево
			|| (arr[max_i + 1] > arr[max_i] && dirs[max_i] == PERM_RIGHT)
			|| (arr[max_i - 1] > arr[max_i] && dirs[max_i] == PERM_LEFT)  //нельзя перепрыгнуть через бОльший
		) 
		{//будем искть дальше
			//переставляем направления
			dirs[0] = PERM_LEFT;
			max_i = -1;
			d--;
		}
		else
		{
			if( (max_i >= len - 1 && dirs[max_i] == PERM_RIGHT) //уперлись вправо
				|| (arr[max_i + 1] > arr[max_i] && dirs[max_i] == PERM_RIGHT)
				|| (arr[max_i - 1] > arr[max_i] && dirs[max_i] == PERM_LEFT)
			)
			{
				dirs[len - 1] = PERM_LEFT;
				max_i = -1;
				d--;
			}
		}*/
		if( -d >= len )
			break;
	}

	return max_i;
}

//перестановка с помощью рекурсии: http://rain.ifmo.ru/cat/view.php/vis/combinations/permutations-2000
int permutation_recur(int *arr, int *dirs, int len)
{//dirs = массив с направлениями для каждого элемента, изначально должен быть заполнен (-1) - все хотят влевао
	//ищем максимальный элемент в массива
	int move_i, tmp, dir;

	move_i = get_movable(arr, dirs, len);

	if( move_i != -1 )
	{
		dir = dirs[move_i];
		tmp = arr[move_i + dir];
		arr[move_i + dir] = arr[move_i];
		arr[move_i] = tmp;
		//направление тоже нужно поменять
		tmp = dirs[move_i + dir];
		dirs[move_i + dir] = dirs[move_i];
		dirs[move_i] = tmp;
		return 1;
	}

	return 0;
}

