/*************************************************************************
    > File Name: glibc_memmem.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 05 Sep 2023 10:07:40 AM CST
 ************************************************************************/

#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <chrono>

#define hash2(p) (((size_t)(p)[0] - ((size_t)(p)[-1] << 3)) % sizeof(shift))

#ifndef CANON_ELEMENT
#define CANON_ELEMENT(c) c
#endif

#define AVAILABLE(h, h_l, j, n_l) ((j) <= (h_l) - (n_l))

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static size_t critical_factorization(const unsigned char *needle, size_t needle_len, size_t *period)
{
    /* Index of last byte of left half, or SIZE_MAX.  */
    size_t max_suffix, max_suffix_rev;
    size_t j;           /* Index into NEEDLE for current candidate suffix.  */
    size_t k;           /* Offset into current period.  */
    size_t p;           /* Intermediate period.  */
    unsigned char a, b; /* Current comparison bytes.  */

    /* Invariants:
       0 <= j < NEEDLE_LEN - 1
       -1 <= max_suffix{,_rev} < j (treating SIZE_MAX as if it were signed)
       min(max_suffix, max_suffix_rev) < global period of NEEDLE
       1 <= p <= global period of NEEDLE
       p == global period of the substring NEEDLE[max_suffix{,_rev}+1...j]
       1 <= k <= p
    */

    /* Perform lexicographic search.  */
    max_suffix = SIZE_MAX;
    j = 0;
    k = p = 1;
    while (j + k < needle_len)
    {
        a = CANON_ELEMENT(needle[j + k]);
        b = CANON_ELEMENT(needle[max_suffix + k]);
        if (a < b)
        {
            /* Suffix is smaller, period is entire prefix so far.  */
            j += k;
            k = 1;
            p = j - max_suffix;
        }
        else if (a == b)
        {
            /* Advance through repetition of the current period.  */
            if (k != p)
                ++k;
            else
            {
                j += p;
                k = 1;
            }
        }
        else /* b < a */
        {
            /* Suffix is larger, start over from current location.  */
            max_suffix = j++;
            k = p = 1;
        }
    }
    *period = p;

    /* Perform reverse lexicographic search.  */
    max_suffix_rev = SIZE_MAX;
    j = 0;
    k = p = 1;
    while (j + k < needle_len)
    {
        a = CANON_ELEMENT(needle[j + k]);
        b = CANON_ELEMENT(needle[max_suffix_rev + k]);
        if (b < a)
        {
            /* Suffix is smaller, period is entire prefix so far.  */
            j += k;
            k = 1;
            p = j - max_suffix_rev;
        }
        else if (a == b)
        {
            /* Advance through repetition of the current period.  */
            if (k != p)
                ++k;
            else
            {
                j += p;
                k = 1;
            }
        }
        else /* a < b */
        {
            /* Suffix is larger, start over from current location.  */
            max_suffix_rev = j++;
            k = p = 1;
        }
    }

    /* Choose the longer suffix.  Return the first byte of the right
       half, rather than the last byte of the left half.  */
    if (max_suffix_rev + 1 < max_suffix + 1)
        return max_suffix + 1;
    *period = p;
    return max_suffix_rev + 1;
}

static void * two_way_long_needle(const unsigned char *haystack, size_t haystack_len,
                                  const unsigned char *needle, size_t needle_len)
{
    size_t i;                           /* Index into current byte of NEEDLE.  */
    size_t j;                           /* Index into current window of HAYSTACK.  */
    size_t period;                      /* The period of the right half of needle.  */
    size_t suffix;                      /* The index of the right half of needle.  */
    size_t shift_table[1U << CHAR_BIT]; /* See below.  */

    /* Factor the needle into two halves, such that the left half is
       smaller than the global period, and the right half is
       periodic (with a period as large as NEEDLE_LEN - suffix).  */
    suffix = critical_factorization(needle, needle_len, &period);

    /* Populate shift_table.  For each possible byte value c,
       shift_table[c] is the distance from the last occurrence of c to
       the end of NEEDLE, or NEEDLE_LEN if c is absent from the NEEDLE.
       shift_table[NEEDLE[NEEDLE_LEN - 1]] contains the only 0.  */
    for (i = 0; i < 1U << CHAR_BIT; i++)
        shift_table[i] = needle_len;
    for (i = 0; i < needle_len; i++)
        shift_table[CANON_ELEMENT(needle[i])] = needle_len - i - 1;

    /* Perform the search.  Each iteration compares the right half
       first.  */
    if (memcmp(needle, needle + period, suffix) == 0)
    {
        /* Entire needle is periodic; a mismatch can only advance by the
       period, so use memory to avoid rescanning known occurrences
       of the period.  */
        size_t memory = 0;
        size_t shift;
        j = 0;
        while (AVAILABLE(haystack, haystack_len, j, needle_len))
        {
            const unsigned char *pneedle;
            const unsigned char *phaystack;

            /* Check the last byte first; if it does not match, then
               shift to the next possible match location.  */
            shift = shift_table[CANON_ELEMENT(haystack[j + needle_len - 1])];
            if (0 < shift)
            {
                if (memory && shift < period)
                {
                    /* Since needle is periodic, but the last period has
                       a byte out of place, there can be no match until
                       after the mismatch.  */
                    shift = needle_len - period;
                }
                memory = 0;
                j += shift;
                continue;
            }
            /* Scan for matches in right half.  The last byte has
               already been matched, by virtue of the shift table.  */
            i = MAX(suffix, memory);
            pneedle = &needle[i];
            phaystack = &haystack[i + j];
            while (i < needle_len - 1 && (CANON_ELEMENT(*pneedle++) == CANON_ELEMENT(*phaystack++)))
                ++i;
            if (needle_len - 1 <= i)
            {
                /* Scan for matches in left half.  */
                i = suffix - 1;
                pneedle = &needle[i];
                phaystack = &haystack[i + j];
                while (memory < i + 1 && (CANON_ELEMENT(*pneedle--) == CANON_ELEMENT(*phaystack--)))
                    --i;
                if (i + 1 < memory + 1)
                    return (void *)(haystack + j);
                /* No match, so remember how many repetitions of period
               on the right half were scanned.  */
                j += period;
                memory = needle_len - period;
            }
            else
            {
                j += i - suffix + 1;
                memory = 0;
            }
        }
    }
    else
    {
        /* The two halves of needle are distinct; no extra memory is
       required, and any mismatch results in a maximal shift.  */
        size_t shift;
        period = MAX(suffix, needle_len - suffix) + 1;
        j = 0;
        while (AVAILABLE(haystack, haystack_len, j, needle_len))
        {
            const unsigned char *pneedle;
            const unsigned char *phaystack;

            /* Check the last byte first; if it does not match, then
               shift to the next possible match location.  */
            shift = shift_table[CANON_ELEMENT(haystack[j + needle_len - 1])];
            if (0 < shift)
            {
                j += shift;
                continue;
            }
            /* Scan for matches in right half.  The last byte has
               already been matched, by virtue of the shift table.  */
            i = suffix;
            pneedle = &needle[i];
            phaystack = &haystack[i + j];
            while (i < needle_len - 1 && (CANON_ELEMENT(*pneedle++) == CANON_ELEMENT(*phaystack++)))
                ++i;
            if (needle_len - 1 <= i)
            {
                /* Scan for matches in left half.  */
                i = suffix - 1;
                pneedle = &needle[i];
                phaystack = &haystack[i + j];
                while (i != SIZE_MAX && (CANON_ELEMENT(*pneedle--) == CANON_ELEMENT(*phaystack--)))
                    --i;
                if (i == SIZE_MAX)
                    return (void *)(haystack + j);
                j += period;
            }
            else
                j += i - suffix + 1;
        }
    }
    return NULL;
}

void *glibc_memmem(const void *haystack, size_t hs_len, const void *needle, size_t ne_len)
{
    const unsigned char *hs = (const unsigned char *)haystack;
    const unsigned char *ne = (const unsigned char *)needle;

    if (ne_len == 0)
        return (void *)hs;
    if (ne_len == 1)
        return (void *)memchr(hs, ne[0], hs_len);

    /* Ensure haystack length is >= needle length.  */
    if (hs_len < ne_len)
        return NULL;

    const unsigned char *end = hs + hs_len - ne_len;

    if (ne_len == 2)
    {
        uint32_t nw = ne[0] << 16 | ne[1], hw = hs[0] << 16 | hs[1];
        for (hs++; hs <= end && hw != nw;)
            hw = hw << 16 | *++hs;
        return hw == nw ? (void *)hs - 1 : NULL;
    }

    /* Use Two-Way algorithm for very long needles.  */
    if (__builtin_expect(ne_len > 256, 0))
        return two_way_long_needle(hs, hs_len, ne, ne_len);

    uint8_t shift[256];
    size_t tmp, shift1;
    size_t m1 = ne_len - 1;
    size_t offset = 0;
    memset(shift, 0, sizeof(shift));
    for (int i = 1; i < m1; i++)
        shift[hash2(ne + i)] = i;

    shift1 = m1 - shift[hash2(ne + m1)];
    shift[hash2(ne + m1)] = m1;

    for (; hs <= end;)
    {
        do
        {
            hs += m1;
            tmp = shift[hash2(hs)];
        } while (tmp == 0 && hs <= end);

        hs -= tmp;
        if (tmp < m1)
            continue;

        if (m1 < 15 || memcmp(hs + offset, ne + offset, 8) == 0)
        {
            if (memcmp(hs, ne, m1) == 0)
                return (void *)hs;

            offset = (offset >= 8 ? offset : m1) - 8;
        }
        hs += shift1;
    }
    return NULL;
}

uint64_t timenow()
{
    std::chrono::steady_clock::time_point tm = std::chrono::steady_clock::now();
    std::chrono::nanoseconds mills = 
        std::chrono::duration_cast<std::chrono::nanoseconds>(tm.time_since_epoch());

    return mills.count();
}

#define NUM_STRINGS     128
#define MAX_LENGTH      1024 * 10
#define SUB_MAX_LEN     32 * 10

char mainStrings[NUM_STRINGS][MAX_LENGTH + 1];
char subStrings[NUM_STRINGS][SUB_MAX_LEN + 1];
void* indexVec[NUM_STRINGS];

void genStringVec()
{
    time_t nTime = time(NULL);
    srand(nTime);

    // 生成随机的主字符串和对应的子字符串
    for (int i = 0; i < NUM_STRINGS; i++) {
        // 生成随机的主字符串
        for (int j = 0; j < MAX_LENGTH; j++) {
            mainStrings[i][j] = 'a' + rand() % 26;
        }
        mainStrings[i][MAX_LENGTH] = '\0';

        int32_t index = rand() % (MAX_LENGTH - SUB_MAX_LEN);

        indexVec[i] = mainStrings[i] + index;

        // 生成对应的子字符串，长度为主字符串长度的一半
        strncpy(subStrings[i], mainStrings[i] + index, SUB_MAX_LEN);
        subStrings[i][SUB_MAX_LEN] = '\0';
        // printf("[%s, %s]\n", mainStrings[i], subStrings[i]);
    }
}

int main(int argc, char **argv)
{
    genStringVec();

    const int32_t recyle = NUM_STRINGS;
    uint64_t timeBegin = timenow();
    for (int32_t i = 0; i < recyle; ++i)
    {
        assert(indexVec[i] == glibc_memmem(mainStrings[i], MAX_LENGTH, subStrings[i], SUB_MAX_LEN));
    }

    uint64_t timeEnd = timenow();
    printf("glibc_memmem spend: %3.2fns\n", (timeEnd - timeBegin) / static_cast<float>(recyle));

    timeBegin = timenow();
    for (int32_t i = 0; i < recyle; ++i)
    {
        assert(indexVec[i] == memmem(mainStrings[i], MAX_LENGTH, subStrings[i], SUB_MAX_LEN));
    }
    timeEnd = timenow();
    printf("memmem spend: %3.2fns\n", (timeEnd - timeBegin) / static_cast<float>(recyle));

    return 0;
}
