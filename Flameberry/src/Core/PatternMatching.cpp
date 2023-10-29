#include "PatternMatching.h"

#include <cstring>
#include <cctype>

namespace Flameberry {
    void calculateLPS(const char* pat, int* lps)
    {
        int len = strlen(pat);
        lps[0] = 0;

        int curr_matched = 0;
        for (int i = 1; i < len; i++)
        {
            if (pat[curr_matched] == pat[i])
                curr_matched++;
            else
                curr_matched = 0;
            lps[i] = curr_matched;
        }
    }

    int kmpSearch(const char* txt, const char* pat, bool ignoreCase)
    {
        int pat_len = strlen(pat);
        int txt_len = strlen(txt);
        int lps[pat_len];
        calculateLPS(pat, lps);

        int i = 0, j = -1;
        while (i < txt_len)
        {
            if (j == pat_len - 1)
                break;

            if (ignoreCase ? (std::tolower(txt[i]) == std::tolower(pat[j + 1])) : (txt[i] == pat[j + 1])) {
                j++;
                i++;
            }
            else {
                if (j == -1)
                    i++;
                else
                    j = lps[j] - 1;
            }
        }
        if (j == pat_len - 1)
            return i - pat_len;
        return -1;
    }
}
