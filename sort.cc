/*************************************************************************
    > File Name: sort.cpp
    > Author: hsz
    > Brief:
    > Created Time: Tue 06 Sep 2022 02:44:51 PM CST
 ************************************************************************/

#include <iostream>
#include <strstream>
#include <vector>
using namespace std;

void Swap(int &a, int &b)
{
    int temp = a;
    a = b;
    b = temp;
}

/* 冒泡排序 */
void BubbleSort(int *a, int n)
{
    for (int i = n - 1; i >= 0; i--)
        for (int j = 0; j < i; j++)
        {
            if (a[j] > a[j + 1])
                Swap(a[j], a[j + 1]);
        }
}

/* 快速排序 */
void QuickSort(int *a, int low, int high)
{
    int middle = a[(low + high) / 2];
    int i = low;
    int j = high;
    while (i <= j)
    {
        while (a[i] < middle)
            i++;
        while (a[j] > middle)
            j--;

        if (i <= j)
        {
            Swap(a[i], a[j]);
            i++;
            j--;
        }
    }

    if (i < high)
        QuickSort(a, i, high);
    if (j > low)
        QuickSort(a, low, j);
}

/* 桶排序， 无相同元素 */
void BucketSortNoSameElement(int *a, int n)
{
    int max = a[0];
    for (int i = 1; i < n; ++i)
    {
        if (max < a[i])
            max = a[i];
    }

    int *emptyBucket = new int[max];
    for (int i = 0; i < max; ++i)
    {
        emptyBucket[i] = 0;
    }

    for (int i = 0; i < n; ++i)
    {
        emptyBucket[a[i] - 1] = a[i];
    }

    int index = 0;

    for (int i = 0; i < max; ++i)
    {
        if (emptyBucket[i] > 0)
            a[index++] = emptyBucket[i];
    }

    delete[] emptyBucket;
}

/* 桶排序， 有相同元素 */
void BucketSortHasSameElement(int *a, int n)
{
    int max = a[0];
    for (int i = 1; i < n; ++i)
    {
        if (max < a[i])
            max = a[i];
    }

    vector<int> *emptyBucket = new vector<int>[max];
    for (int i = 0; i < n; ++i)
    {
        emptyBucket[a[i] - 1].push_back(a[i]);
    }

    int index = 0;
    for (int i = 0; i < max; ++i)
    {
        vector<int>::iterator iter = emptyBucket[i].begin();
        for (; iter != emptyBucket[i].end(); ++iter)
        {
            a[index++] = *iter;
        }
    }

    delete[] emptyBucket;
}

/* 插入排序 */
void InsertionSort(int *a, int n)
{
    for (int i = 1; i < n; ++i)
    {
        int t = a[i];
        int j = i;
        while ((j > 0) && (a[j - 1] > t))
        {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = t;
    }
}

/* 希尔排序, 插入排序变种 */
void ShellSort(int *a, int n)
{
    int inc = 0;
    for (inc = 1; inc <= n / 9; inc = 3 * inc + 1)
        ;
    for (; inc > 0; inc /= 3)
    {
        for (int i = inc + 1; i <= n; i += inc)
        {
            int t = a[i - 1];
            int j = i;
            while ((j > inc) && (a[j - inc - 1] > t))
            {
                a[j - 1] = a[j - inc - 1];
                j -= inc;
            }
            a[j - 1] = t;
        }
    }
}

/* 基数排序 */
typedef vector<unsigned int> input_type;
void RadixSort(input_type &x)
{
    if (x.empty())
        return;

    typedef vector<vector<input_type::value_type>> buckets_type;
    buckets_type buckets;

    buckets.resize(10);

    int pow10 = 1; // pow10 holds powers of 10 (1, 10, 100, ...)

    input_type::value_type max = *std::max(x.begin(), x.end());

    for (; max != 0; max /= 10, pow10 *= 10)
    {
        for (input_type::const_iterator elem = x.begin(); elem != x.end(); ++elem)
        {
            size_t const bucket_num = (*elem / pow10) % 10;
            buckets[bucket_num].push_back(*elem);
        }

        // transfer results of buckets back into main array
        input_type::iterator store_pos = x.begin();

        for (buckets_type::iterator bucket = buckets.begin(); bucket != buckets.end(); ++bucket)
        {
            for (buckets_type::value_type::const_iterator bucket_elem = bucket->begin();
                 bucket_elem != bucket->end(); ++bucket_elem)
            {
                *store_pos++ = *bucket_elem;
            }
            bucket->clear();
        }
    }
}

/* 鸽巢排序 */
void PigeonholeSort(int *a, int n)
{
    int max = a[0];
    for (int i = 1; i < n; ++i)
    {
        if (max < a[i])
            max = a[i];
    }

    int *pigeonHole = new int[max + 1];
    for (int i = 0; i < n; ++i)
    {
        pigeonHole[a[i]] = 0;
    }

    for (int i = 0; i < n; ++i)
    {
        pigeonHole[a[i]]++;
    }

    int index = 0;

    for (int i = 0; i <= max; ++i)
    {
        for (int j = 0; j < pigeonHole[i]; ++j)
        {
            a[index++] = i;
        }
    }

    delete[] pigeonHole;
}

/* 合并排序 */
void Merge(vector<int> left, vector<int> right, vector<int> &result)
{
    result.clear();
    int i = 0, j = 0;
    while (i < left.size() && j < right.size())
    {
        if (left[i] < right[j])
        {
            result.push_back(left[i++]);
        }
        else
        {
            result.push_back(right[j++]);
        }
    }

    // 还有左元素，没有右元素
    while (i < left.size())
    {
        result.push_back(left[i++]);
    }

    //还有右元素，没有左元素
    while (j < right.size())
    {
        result.push_back(right[j++]);
    }
}

void MergeSort(vector<int> &a)
{
    if (a.size() <= 1)
        return;
    vector<int> left;
    vector<int> right;
    int middle = a.size() / 2;

    for (int i = 0; i < middle; ++i)
    {
        left.push_back(a[i]);
    }

    for (int i = middle; i < a.size(); ++i)
    {
        right.push_back(a[i]);
    }

    MergeSort(left);
    MergeSort(right);

    Merge(left, right, a);
}

/* 选择排序 */
void SelectionSort(int *a, int n)
{
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
        {
            if (a[j] < a[i])
            {
                Swap(a[j], a[i]);
            }
        }
}

/* 计数排序 */
void CountingSort(int *a, int n)
{
    int max = a[0];
    int min = a[0];

    for (int i = 1; i < n; ++i)
    {
        if (a[i] > max)
            max = a[i];
        if (a[i] < min)
            min = a[i];
    }

    int range = max - min + 1;
    int *count = new int[range];

    for (int i = 0; i < range; ++i)
    {
        count[i] = 0;
    }

    for (int i = 0; i < n; ++i)
    {
        count[a[i] - min]++;
    }

    int index = 0;
    for (int i = min; i <= max; i++)
        for (int j = 0; j < count[i - min]; ++j)
            a[index++] = i;

    delete[] count;
}
